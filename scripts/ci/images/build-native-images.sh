#!/bin/bash

if ! which docker-squash > /dev/null
then
  echo "docker-squash not found in PATH."
  echo
  echo "This relies on docker-squash from"
  echo "https://github.com/goldmann/docker-squash"
  echo "to be present in your path"
fi

function build_partially_squashed_image()
{
  local IMAGE_FROM=$1
  local IMAGE_TO=$2

  echo "${IMAGE_TO}"
  docker build -t ornladios/adios2:ci-${IMAGE_TO} ${IMAGE_TO}

  docker-squash \
    -f ornladios/adios2:ci-${IMAGE_FROM} \
    -t ornladios/adios2:ci-${IMAGE_TO} \
    ornladios/adios2:ci-${IMAGE_TO}
}

if [ "${ADIOS2_DOCKER_BUILD}" != "0" ]
then

echo "************************************************************"
echo "* Building fully squashed root base images                 *"
echo "************************************************************"
ROOT_BASE_IMAGES="el7-base suse-nvphcsdk-base fedora-sanitizers-base debian-sid"
for IMAGE in ${ROOT_BASE_IMAGES}
do
  echo "${IMAGE}"
  docker build --no-cache --squash -t ornladios/adios2:ci-${IMAGE} ${IMAGE}
  echo
done

echo "************************************************************"
echo "* Building partially squashed intermediate base images     *"
echo "************************************************************"
INTERMEDIATE_IMAGES="el7-base,el7-gnu8-ohpc-base el7-base,el7-intel-ohpc-base"
for IMAGE_PAIR in ${INTERMEDIATE_IMAGES}
do
  echo "${IMAGE_PAIR%,*} -> ${IMAGE_PAIR#*,}"
  build_partially_squashed_image ${IMAGE_PAIR%,*} ${IMAGE_PAIR#*,}
  echo
done

echo "************************************************************"
echo "* Building partially squashed final images                 *"
echo "************************************************************"
LEAF_IMAGES="el7-base,el7 el7-gnu8-ohpc-base,el7-gnu8-ohpc el7-gnu8-ohpc-base,el7-gnu8-openmpi-ohpc el7-intel-ohpc-base,el7-intel-ohpc el7-intel-ohpc-base,el7-intel-openmpi-ohpc suse-nvphcsdk-base,suse-pgi suse-nvphcsdk-base,suse-nvphcsdk-openmpi fedora-sanitizers-base,fedora-asan fedora-sanitizers-base,fedora-ubsan"
for IMAGE_PAIR in ${LEAF_IMAGES}
do
  echo "${IMAGE_PAIR%,*} -> ${IMAGE_PAIR#*,}"
  build_partially_squashed_image ${IMAGE_PAIR%,*} ${IMAGE_PAIR#*,}
  echo
done

fi


if [ "${ADIOS2_DOCKER_PUSH}" != "0" ]
then

echo "************************************************************"
echo "* Push all images                                          *"
echo "************************************************************"
ALL_IMAGES="el7-base el7 el7-gnu8-ohpc-base el7-gnu8-ohpc el7-gnu8-openmpi-ohpc el7-intel-ohpc-base el7-intel-ohpc el7-intel-openmpi-ohpc suse-nvphcsdk-base suse-pgi suse-nvphcsdk-openmpi fedora-sanitizers-base fedora-asan fedora-ubsan debian-sid"
for IMAGE in ${ALL_IMAGES}
do
  echo "${IMAGE}"
  docker push ornladios/adios2:ci-${IMAGE}
  echo
done

fi
