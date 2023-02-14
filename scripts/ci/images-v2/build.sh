#!/bin/bash

function message1() {
  echo ""
  echo "****************************************"
  printf '* %-36s *\n' "$1"
  echo "****************************************"
  echo ""
}

function build_squash() {
  local FROM=$1
  local TO=$2
  local DF=$3
  local ARGS="$4"

  echo "*"
  echo "* Building $TO"
  echo "*"
  if ! docker build --rm $ARGS -t $TO -f $DF .
  then
    echo "Error: Failed to build $TO image"
    return 1
  fi
  echo "*"
  echo "* Squashing:"
  echo "*   from $FROM"
  echo "*     to $TO"
  echo "*     as $TO"
  echo "*"
  docker-squash -f $FROM -t $TO $TO
  return 0
}

enable_base=0
enable_inter=0
enable_gcc=0
enable_clang=0
enable_intel=0
enable_nvhpc=0
enable_serial=0
enable_mpi=0

while [ $# -gt 0 ]
do
  case $1 in
    base) enable_base=1;;
    no_base) enable_base=0;;
    inter) enable_inter=1;;
    no_inter) enable_inter=0;;
    gcc) enable_gcc=1;;
    no_gcc) enable_gcc=0;;
    clang) enable_clang=1;;
    no_clang) enable_clang=0;;
    intel) enable_intel=1;;
    no_intel) enable_intel=0;;
    nvhpc) enable_nvhpc=1;;
    no_nvhpc) enable_nvhpc=0;;
    serial) enable_serial=1;;
    no_serial) enable_serial=0;;
    mpi) enable_mpi=1;;
    no_mpi) enable_mpi=0;;
    *)
      echo "Error: Unknown option: $1"
      exit 1;;
  esac
  shift
done

if [ $enable_base -eq 1 ]
then
  message1 "Building ci base image"
  if ! build_squash \
    almalinux:8 \
    ornladios/adios2:ci-spack-el8-base \
    Dockerfile.ci-spack-el8-base
  then
    echo "Error: Failed to build base image"
    exit 2
  fi
fi


if [ $enable_inter -eq 1 ]
then
  for COMPILER in gcc clang intel nvhpc
  do
    enabled_var="enable_$COMPILER"
    if [ ${!enabled_var} -eq 1 ]
    then
      message1 "Building $COMPILER base image"
      if ! build_squash \
        ornladios/adios2:ci-spack-el8-base \
        ornladios/adios2:ci-spack-el8-${COMPILER}-base \
        Dockerfile.ci-spack-el8-${COMPILER}-base
      then
        echo "Error: Failed to build ${COMPILER}-base image"
        exit 3
      fi
    fi
  done
fi

if [ $enable_serial -eq 0 ] && [ $enable_mpi -eq 0 ]
then
  exit
fi

for COMPILER in gcc clang intel nvhpc
do
  enabled_var="enable_$COMPILER"
  if [[ ${!enabled_var} -eq 0 ]]
  then
    continue
  fi

  if [ $enable_serial -eq 1 ]
  then
    message1 "Building $COMPILER serial image"
    build_squash \
      ornladios/adios2:ci-spack-el8-${COMPILER}-base \
      ornladios/adios2:ci-spack-el8-$COMPILER-serial \
      Dockerfile.ci-spack-el8-serial \
      "--build-arg COMPILER=$COMPILER"
  fi
  if [ $enable_mpi -eq 1 ]
  then
    message1 "Building $COMPILER mpi image"
    build_squash \
      ornladios/adios2:ci-spack-el8-$COMPILER-serial \
      ornladios/adios2:ci-spack-el8-$COMPILER-mpi \
      Dockerfile.ci-spack-el8-mpi \
      "--build-arg COMPILER=$COMPILER"
  fi
done
