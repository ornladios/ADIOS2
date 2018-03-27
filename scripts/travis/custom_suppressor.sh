#!/usr/bin/env bash

SUPPRESSIONS_FILE=$1

declare -a suppressions

# Take in path to check and array of regular expressions representing
# paths to ignore
ignore_path() {
  path_to_check=$1
  #suppression_list=$2

  for suppr in ${suppressions[@]}; do
    if [[ "$path_to_check" =~ $suppr ]]
    then
      return 1
    fi
  done

  return 0
}

# Read in the suppressions file, ignore blank or commented lines
IDX=0
while read line
do
  if [ -n "$line" ] && ! [[ "$line" =~ ^\/\/ ]]; then
    # echo "Adding suppression at index ${IDX}: ${line}"
    suppressions[IDX++]=$line
  fi
done < "$SUPPRESSIONS_FILE"

# Just verify what we read in as suppressions
# echo "Suppressions:"
# for s in ${suppressions[@]}; do
#   echo "  $s"
# done
