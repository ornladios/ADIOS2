#!/bin/bash

set -ex

########################################
# ppc64le CentOS 7 emulation base image
########################################
docker build --squash \
  --build-arg TARGET_ARCH_SYSTEM=ppc64le \
  --build-arg TARGET_ARCH_DOCKER=ppc64le \
  --build-arg TARGET_CPU=power8 \
  -t ornladios/adios2:ci-x86_64-power8-el7 \
  emu-el7
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7

######################################
 ppc64le CI base image
######################################
docker build \
  --build-arg TARGET_CPU=power8 \
  -t ornladios/adios2:ci-x86_64-power8-el7-base \
  emu-el7-base
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7 \
  -t ornladios/adios2:ci-x86_64-power8-el7-base \
  ornladios/adios2:ci-x86_64-power8-el7-base
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-base"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-base
