#!/usr/bin/env bash

# Read suppressions file and define ignore_path function
source ${SUPPRESSOR} ${SUPPRESSIONS}

if [ -z "${DO_CL_ANALYZE}" ]
then
  g++ "$@"
else
  args=$@
  tmpFile=$(mktemp)
  ${CXX_ANALYZER} "$@" >>${tmpFile} 2>&1
  regex='-c["[:space:]]+([^"[:space:]]+)["[:space:]]*'
  if [[ "$args" =~ $regex ]]
  then
    path=${BASH_REMATCH[1]}
    ignore_path $path
    if [ $? == 0 ]
    then
      contents=$(cat ${tmpFile})
      warning=$(echo ${contents} | grep -i ": warning: ")
      if [ -n "${warning}" ]
      then
        (>&2 echo "CLANGANALYZER ERROR CATCH: ${contents}")
        exit 1
      else
        echo "No problems found in ${path}"
      fi
    fi
  fi
  rm ${tmpFile}
fi
