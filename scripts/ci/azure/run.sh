#!/bin/bash --login

joblabel="${SYSTEM_JOBNAME//_/-}"

export CI_SITE_NAME="Azure Pipelines"
if [ -n "${SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}" ]
then
  export CI_BUILD_NAME="pr${SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}_${SYSTEM_PULLREQUEST_SOURCEBRANCH}_${joblabel}"
  export CI_COMMIT_REF=${SYSTEM_PULLREQUEST_SOURCECOMMITID}
else
  export CI_BUILD_NAME="${BUILD_SOURCEBRANCHNAME}_${joblabel}"
  export CI_COMMIT_REF=${BUILD_SOURCEVERSION}
fi
if [[ "${AGENT_OS}" =~ "Windows" ]]
then
  export CI_ROOT_DIR="${PIPELINE_WORKSPACE//\\//}"
  export CI_SOURCE_DIR="${BUILD_SOURCESDIRECTORY//\\//}"
  export CI_BIN_DIR="${BUILD_BINARIESDIRECTORY//\\//}/${joblabel}"
else
  export CI_ROOT_DIR="${PIPELINE_WORKSPACE}"
  export CI_SOURCE_DIR="${BUILD_SOURCESDIRECTORY}"
  export CI_BIN_DIR="${BUILD_BINARIESDIRECTORY}/${joblabel}"
fi


STEP=$1
CTEST_SCRIPT=scripts/ci/cmake/ci-${joblabel}.cmake

# Update and Test steps enable an extra step
CTEST_STEP_ARGS=""
case ${STEP} in
  update) CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_checkout=ON" ;;
  test) CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_end=ON" ;;
esac
CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_${STEP}=ON"

if [ -x /opt/cmake/bin/ctest ]
then
  CTEST=/opt/cmake/bin/ctest
elif [ -s /Applications/CMake.app/Contents/bin/ctest ]
then
  CTEST=/Applications/CMake.app/Contents/bin/ctest
else
  CTEST=ctest
fi

# macOS tmpdir issues
# See https://github.com/open-mpi/ompi/issues/6518
if [[ "{AGENT_OS}" =~ "Darwin" ]]
then
  export TMPDIR=/tmp
fi

# OpenMPI specific setup
if [[ "${SYSTEM_JOBNAME}" =~ openmpi ]]
then
  # Workaround to quiet some warnings from OpenMPI
  export OMPI_MCA_btl_base_warn_component_unused=0
  export OMPI_MCA_btl_vader_single_copy_mechanism=none

  # https://github.com/open-mpi/ompi/issues/6518
  export OMPI_MCA_btl=self,tcp

  # Enable overscription in OpenMPI
  export OMPI_MCA_rmaps_base_oversubscribe=1
  export OMPI_MCA_hwloc_base_binding_policy=none
fi

# Make sure staging tests use localhost
export ADIOS2_IP=127.0.0.1

echo "**********Env Begin**********"
env | sort
echo "**********Env End************"

echo "**********CTest Begin**********"
${CTEST} --version
echo ${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
RET=$?
echo "**********CTest End************"

exit ${RET}
