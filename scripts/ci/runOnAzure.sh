#!/bin/bash --login

joblabel="${SYSTEM_JOBNAME//_/-}"

export CI_SITE_NAME="Azure Pipelines"
if [ -n "${SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}" ]
then
  export CI_BUILD_NAME="pr${SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}_${SYSTEM_PULLREQUEST_SOURCEBRANCH}_${BUILD_BUILDID}_${joblabel}"
  export CI_COMMIT=${SYSTEM_PULLREQUEST_SOURCECOMMITID}
else
  export CI_BUILD_NAME="${BUILD_SOURCEBRANCHNAME}_${BUILD_BUILDID}_${joblabel}"
  export CI_COMMIT=${BUILD_SOURCEVERSION}
fi
export CI_ROOT_DIR="${PIPELINE_WORKSPACE}"
export CI_SOURCE_DIR="${BUILD_SOURCESDIRECTORY}"
export CI_BIN_DIR="${BUILD_BINARIESDIRECTORY}/${joblabel}"


STEP=$1
CTEST_SCRIPT=scripts/ci/cmake/ci-${joblabel}.cmake

# Update and Test steps enable an extra step
CTEST_STEP_ARGS=""
case ${STEP} in
  update) CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_checkout=ON" ;;
  test) CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_end=ON" ;;
esac
CTEST_STEP_ARGS="${CTEST_STEP_ARGS} -Ddashboard_do_${STEP}=ON"

# Workaround to quiet some warnings from OpenMPI
export OMPI_MCA_btl_base_warn_component_unused=0
export OMPI_MCA_btl_vader_single_copy_mechanism=none

# Enable overscription in OpenMPI
export OMPI_MCA_rmaps_base_oversubscribe=1
export OMPI_MCA_hwloc_base_binding_policy=none

echo "**********Env Begin**********"
env | sort
echo "**********Env End************"

if [ -x /opt/cmake/bin/ctest ]
then
  CTEST=/opt/cmake/bin/ctest
else
  CTEST=ctest
fi

echo "**********CTest Begin**********"
${CTEST} --version
echo ${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF ${CTEST_STEP_ARGS}
RET=$?
echo "**********CTest End************"
exit ${RET}
