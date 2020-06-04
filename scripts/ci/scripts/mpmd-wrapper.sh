#!/bin/bash

function usage()
{
  echo "Usage: mpi-mpmd-wrapper <MPI_RANK_ENV_VAR> -n P ex1 ex1a1 .. ex1aN : -n Q ex2 ex2a1 ... ex2aN : ..."

}

eval RANK=\$$1
if [ -z "${RANK}" ]
then
  echo "Error:  Unable to determine rank from $1"
  exit 1
fi
shift

BUCKETS_N=()
BUCKETS_CMD=()

while [ $# -gt 0 ]
do
  N=
  if [ $1 = "-n" ]
  then
    N=$2
    shift 2
  elif [[ $1 =~ ^-n[0-9]+ ]]
  then
    N=$(expr match "$1" "-n\([0-9]\+\)")
    shift
  else
    echo "Error: The first argument in each group must be -n<#>"
    exit 1
  fi

  CMD=
  while [ $# -gt 0 ]
  do
    if [ "$1" = ":" ]
    then
      break
    fi
    CMD="${CMD} $1"
    shift
  done
  shift
  
  BUCKETS_N+=( $N )
  BUCKETS_CMD+=( "$(echo ${CMD})" )
done

NNEXT=0
for i in ${!BUCKETS_N[@]}
do
  NNEXT=$((${NNEXT} + ${BUCKETS_N[$i]}))
  if [ ${RANK} -lt ${NNEXT} ]
  then
    exec ${BUCKETS_CMD[$i]}
    exit $?
  fi
done
exit 0
