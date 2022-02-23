function message1() {
  echo ""
  echo "****************************************"
  printf '* %-36s *\n' "$1"
  echo "****************************************"
  echo ""
}

function build_squash()
{
  local FROM=$1
  local TO=$2
  local DOCKERFILE=$3
  local ARGS="$4"

  if [ "${ADIOS2_CI_NO_SQUASH}" = "1" ]
  then
    echo "*"
    echo "* Building ${TO}"
    echo "*"
    if ! docker build --rm ${ARGS} -t ${TO} -f ${DOCKERFILE} .
    then
      echo "Error: Failed to build ${TO} image"
      return 1
    fi
  else
    echo "*"
    echo "* Building ${TO}-tmp"
    echo "*"
    if ! docker build --rm ${ARGS} -t ${TO}-tmp -f ${DOCKERFILE} .
    then
      echo "Error: Failed to build ${TO}-tmp image"
      return 1
    fi
    echo "*"
    echo "* Squashing:"
    echo "*   from ${FROM}"
    echo "*     to ${TO}-tmp"
    echo "*     as ${TO}"
    echo "*"
    docker-squash -c -f ${FROM} -t ${TO} ${TO}-tmp
  fi
  return 0
}

function build_leafs()
{
  local IMG_BASE=$1
  local SPACK_ID=$2

  message1 "Building ${IMG_BASE}-serial image"
  if ! build_squash \
    ornladios/adios2:ci-spack-el8-${IMG_BASE}-base \
    ornladios/adios2:ci-spack-el8-${IMG_BASE}-serial \
    Dockerfile.ci-spack-el8-leaf \
    "--build-arg COMPILER_IMG_BASE=${IMG_BASE} --build-arg COMPILER_SPACK_ID=${SPACK_ID} --build-arg EXTRA_VARIANTS=~mpi"
  then
    echo "Error: Failed to build ${IMG_BASE}-serial image"
    return 1
  fi

  message1 "Building ${IMG_BASE}-mpi image"
  if ! build_squash \
    ornladios/adios2:ci-spack-el8-${IMG_BASE}-base \
    ornladios/adios2:ci-spack-el8-${IMG_BASE}-mpi \
    Dockerfile.ci-spack-el8-leaf \
    "--build-arg COMPILER_IMG_BASE=${IMG_BASE} --build-arg COMPILER_SPACK_ID=${SPACK_ID} --build-arg EXTRA_VARIANTS=+mpi"
  then
    echo "Error: Failed to build ${IMG_BASE}-mpi image"
    return 2
  fi
  return 0
}
