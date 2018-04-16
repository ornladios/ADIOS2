#!/usr/bin/env bash

# Read suppressions file and define ignore_path function
source ${SUPPRESSOR} ${SUPPRESSIONS}

if [ -z "${DOCPPCHECK}" ]
then
  g++ "$@"
else
  args=$@
  regex='-c["[:space:]]+([^"[:space:]]+)["[:space:]]*'
  if [[ "$args" =~ $regex ]]
  then
    path=${BASH_REMATCH[1]}
    ignore_path $path
    if [ $? == 0 ]        # check if we should ignore
    then
      { RESULT=$(${CPPCHECK_EXE} --enable=style --template="CPPCHECK ERROR CATCH - {file}:{line}: ({severity}) => {id} {message}" "$path" 2>&1 1>&3-) ;} 3>&1
      if [[ "$RESULT" =~ "CPPCHECK ERROR CATCH" ]]; then
        (>&2 echo "${RESULT}")
        exit 1
      fi
    fi
  fi
fi
