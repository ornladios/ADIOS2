#!/bin/bash

########################################
# CentOS 7
########################################
if [ "${ADIOS2_DOCKER_BUILD}" != "0" ]
then
  docker build --rm \
    --build-arg DISTRO=centos7 \
    -t ornladios/adios2:ci-el7-spack \
    spack
  docker-squash \
    -f spack/centos7 \
    -t ornladios/adios2:ci-el7-spack \
    ornladios/adios2:ci-el7-spack
fi
if [ "${ADIOS2_DOCKER_PUSH}" != "0" ]
then
  echo ""
  echo "Pushing ornladios/adios2:ci-el7-spack"
  echo ""
  docker push ornladios/adios2:ci-el7-spack
fi

########################################
# Ubuntu 18.04
########################################
if [ "${ADIOS2_DOCKER_BUILD}" != "0" ]
then
  docker build --rm \
    --build-arg DISTRO=ubuntu-bionic \
    -t ornladios/adios2:ci-ubuntu1804-spack \
    spack
  docker-squash \
    -f spack/ubuntu-bionic \
    -t ornladios/adios2:ci-ubuntu1804-spack \
    ornladios/adios2:ci-ubuntu1804-spack
fi
if [ "${ADIOS2_DOCKER_PUSH}" != "0" ]
then
  echo ""
  echo "Pushing ornladios/adios2:ci-ubuntu1804-spack"
  echo ""
  docker push ornladios/adios2:ci-ubuntu1804-spack
fi
