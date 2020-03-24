#!/usr/bin/env bash

if [ -z "${SOURCE_DIR}" ]
then
  echo "Error: SOURCE_DIR is empty or undefined"
  exit 1
fi
if [ -n "${TRAVIS_PULL_REQUEST_SHA}" ]
then
  COMMITISH="pr${TRAVIS_PULL_REQUEST}"
else
  COMMITISH="${TRAVIS_BRANCH}"
fi

cd ${SOURCE_DIR}

case ${BUILD_MATRIX_ENTRY} in
  docker-ubuntu1804)
    if ! docker build --build-arg adios_ver=${COMMITISH} --build-arg ubuntu_ver=18.04 scripts/docker/images/ubuntu ; then
      exit 1;
    fi
    ;;
  docker-ubuntu1910)
    if ! docker build --build-arg adios_ver=${COMMITISH} --build-arg ubuntu_ver=19.10 scripts/docker/images/ubuntu ; then
      exit 1;
    fi
    ;;
  docker-centos7)
    if ! docker build --build-arg adios_ver=${COMMITISH} scripts/docker/images/centos7 ; then
      exit 1;
    fi
    ;;
  docker-centos8)
    if ! docker build --build-arg adios_ver=${COMMITISH} scripts/docker/images/centos8 ; then
      exit 1;
    fi
    ;;
  *)
    echo "Error: BUILD_MATRIX_ENTRY is set to an unknown docker image"
    exit 1;
    ;;
esac

exit 0
