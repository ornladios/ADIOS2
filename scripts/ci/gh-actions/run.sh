#!/bin/bash --login

if [[ "${GITHUB_JOB}" =~ emu ]]
then
  export CI_SITE_NAME="GitHub Actions (QEMU)"
else
  export CI_SITE_NAME="GitHub Actions"
fi

if [ "${GITHUB_EVENT_NAME}" = "pull_request" ]
then
  GH_PR_NUMBER=$(expr "${GITHUB_REF}" : 'refs/pull/\([^/]*\)')
  export CI_BUILD_NAME="pr${GH_PR_NUMBER}_${GITHUB_HEAD_REF}_${GH_YML_JOBNAME}"
else
  export CI_BUILD_NAME="${GITHUB_REF#refs/heads/}_${GH_YML_JOBNAME}"
fi
if [[ "${GH_YML_OS}" =~ "Windows" ]]
then
  export CI_ROOT_DIR="${GITHUB_WORKSPACE//\\//}/.."
  export CI_SOURCE_DIR="${GITHUB_WORKSPACE//\\//}"
else
  export CI_ROOT_DIR="${GITHUB_WORKSPACE}/.."
  export CI_SOURCE_DIR="${GITHUB_WORKSPACE}"
fi
export CI_BIN_DIR="${CI_ROOT_DIR}/${GH_YML_JOBNAME}"

STEP=$1
CTEST_SCRIPT=scripts/ci/cmake/ci-${GH_YML_JOBNAME}.cmake

# Update and Test steps enable an extra step
CTEST_STEP_ARGS=""
case ${STEP} in
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

# Don't rely on the container's storage for tmp
export TMPDIR="${RUNNER_TEMP}/tmp"
mkdir -p "${TMPDIR}"

# OpenMPI specific setup and workarounds
if [[ "${GH_YML_JOBNAME}" =~ (openmpi|smpi) ]]
then
  # Quiet some warnings from OpenMPI
  export OMPI_MCA_btl_base_warn_component_unused=0
  export OMPI_MCA_btl_vader_single_copy_mechanism=none

  # Force only shared mem backends
  export OMPI_MCA_btl="self,vader"

  # Workaround for open-mpi/ompi#7516
  export OMPI_MCA_gds=hash

  # Workaround for open-mpi/ompi#5798
  export OMPI_MCA_btl_vader_backing_directory="/tmp"

  # Enable overscription in OpenMPI
  export OMPI_MCA_rmaps_base_oversubscribe=1
  export OMPI_MCA_hwloc_base_binding_policy=none

  # Disable OpenMPI rsh launching
  export OMPI_MCA_plm_rsh_agent=false

  # Disable cuda warnings
  export OMPI_MCA_opal_warn_on_missing_libcuda=0
fi


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
