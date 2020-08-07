#!/bin/bash

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

########################################
# ppc64le CI base image
########################################
docker build --squash \
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

########################################
# XL base image
########################################
docker build \
  -t ornladios/adios2:ci-x86_64-power8-el7-xl-base \
  power8-el7-xl-base
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7-base \
  -t ornladios/adios2:ci-x86_64-power8-el7-xl-base \
  ornladios/adios2:ci-x86_64-power8-el7-xl-base
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-xl-base"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-xl-base

########################################
# XL builder image
########################################
docker build \
  -t ornladios/adios2:ci-x86_64-power8-el7-xl \
  --build-arg COMPILER=xl \
  power8-el7-leaf
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7-xl-base \
  -t ornladios/adios2:ci-x86_64-power8-el7-xl \
  ornladios/adios2:ci-x86_64-power8-el7-xl
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-xl"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-xl

########################################
# XL + MPI builder image
########################################
docker build \
  -t ornladios/adios2:ci-x86_64-power8-el7-xl-smpi \
  --build-arg COMPILER=xl \
  power8-el7-leaf-smpi
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7-xl-base \
  -t ornladios/adios2:ci-x86_64-power8-el7-xl-smpi \
  ornladios/adios2:ci-x86_64-power8-el7-xl-smpi
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-xl-smpi"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-xl-smpi

########################################
# PGI base image
########################################
docker build \
  -t ornladios/adios2:ci-x86_64-power8-el7-pgi-base \
  power8-el7-pgi-base
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7-base \
  -t ornladios/adios2:ci-x86_64-power8-el7-pgi-base \
  ornladios/adios2:ci-x86_64-power8-el7-pgi-base
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-pgi-base"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-pgi-base

########################################
# PGI builder image
########################################
docker build \
  -t ornladios/adios2:ci-x86_64-power8-el7-pgi \
  --build-arg COMPILER=pgi \
  power8-el7-leaf
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7-pgi-base \
  -t ornladios/adios2:ci-x86_64-power8-el7-pgi \
  ornladios/adios2:ci-x86_64-power8-el7-pgi
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-pgi"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-pgi

########################################
# PGI + MPI builder image
########################################
docker build \
  -t ornladios/adios2:ci-x86_64-power8-el7-pgi-smpi \
  --build-arg COMPILER=pgi --build-arg HDF5_ARGS="-DMPI_C_COMPILER=mpipgicc" \
  power8-el7-leaf-smpi
docker-squash \
  -f ornladios/adios2:ci-x86_64-power8-el7-pgi-base \
  -t ornladios/adios2:ci-x86_64-power8-el7-pgi-smpi \
  ornladios/adios2:ci-x86_64-power8-el7-pgi-smpi
echo ""
echo "Pushing ornladios/adios2:ci-x86_64-power8-el7-pgi-smpi"
echo ""
docker push ornladios/adios2:ci-x86_64-power8-el7-pgi-smpi
