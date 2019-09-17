#!/bin/bash

API_BASE="https://api.github.com/repos/ornladios/adios2"
USER=${STATUS_ROBOT_NAME}
TOKEN=${STATUS_ROBOT_KEY}
COMMIT=${CIRCLE_SHA1}
CDASH_STATUS_CONTEXT="cdash"

# Workaround to quiet some warnings from OpenMPI and CircleCI
export OMPI_MCA_btl_base_warn_component_unused=0
export OMPI_MCA_btl_vader_single_copy_mechanism=none

# Enable overscription in OpenMPI
export OMPI_MCA_rmaps_base_oversubscribe=1
export OMPI_MCA_hwloc_base_binding_policy=none

if [ -x /usr/bin/python2 ]
then
  PYTHON_EXECUTABLE=/usr/bin/python2
elif [ -x /usr/bin/python3 ]
then
  PYTHON_EXECUTABLE=/usr/bin/python3
elif [ -x /usr/bin/python ]
then
  PYTHON_EXECUTABLE=/usr/bin/python
else
  PYTHON_EXECUTABLE=python
fi

build_status_body() {
  cat <<EOF
{
  "state": "success",
  "target_url": "https://open.cdash.org/index.php?compare1=61&filtercount=1&field1=revision&project=ADIOS&showfilters=0&limit=100&value1=${COMMIT}&showfeed=0",
  "description": "Build and test results available on CDash",
  "context": "${CDASH_STATUS_CONTEXT}"
}
EOF
}

check_and_post_status() {
  PYTHON_SCRIPT="${SOURCE_DIR}/scripts/ci/findStatus.py"
  curl -u "${STATUS_ROBOT_NAME}:${STATUS_ROBOT_KEY}" "${API_BASE}/commits/${COMMIT}/statuses" | ${PYTHON_EXECUTABLE} ${PYTHON_SCRIPT} --context ${CDASH_STATUS_CONTEXT}
  if [ $? -ne 0 ]
  then
    echo "Need to post a status for context ${CDASH_STATUS_CONTEXT}"
    postBody="$(build_status_body)"
    postUrl="${API_BASE}/statuses/${COMMIT}"
    curl -u "${OWNER}:${TOKEN}" "${postUrl}" -H "Content-Type: application/json" -H "Accept: application/vnd.github.v3+json" -d "${postBody}"
  fi
}

get_real_branch_name() {
  REALBRANCH="${CIRCLE_BRANCH}"
  if [ -n "${CIRCLE_PR_NUMBER}" ]
  then
    APIURL="${API_BASE}/pulls/${CIRCLE_PR_NUMBER}"
    RESULT="$(curl -s ${APIURL} | ${PYTHON_EXECUTABLE} -c "import sys, json; print(json.load(sys.stdin)['head']['ref'])" 2> /dev/null)"
    if [ $? -eq 0 ]
    then
      REALBRANCH="${RESULT}"
    fi
  fi
}

check_var() {
  if [ -z "${!1}" ]
  then
    echo "Error: The $1 environment variable is undefined"
    exit 1
  fi
}

check_var CIRCLE_WORKING_DIRECTORY
check_var CIRCLE_BRANCH
check_var CIRCLE_JOB
check_var CIRCLE_BUILD_NUM

if [ ! "${CUSTOM_BUILD_NAME}" ]
then
  get_real_branch_name

  LINETOSAVE="export CUSTOM_BUILD_NAME=pr${CIRCLE_PR_NUMBER}_${REALBRANCH}_${CIRCLE_BUILD_NUM}_${CIRCLE_JOB}"

  # Set the custom build name for this step
  eval $LINETOSAVE

  # Also make sure it will get set for the following steps
  echo "${LINETOSAVE}" >> $BASH_ENV
fi

SOURCE_DIR=${CIRCLE_WORKING_DIRECTORY}/source
CTEST_SCRIPT="${SOURCE_DIR}/scripts/ci/cmake/circle_${CIRCLE_JOB}.cmake"

if [ ! -f "${CTEST_SCRIPT}" ]
then
  echo "Unable to find CTest script $(basename ${CTEST_SCRIPT})"
  exit 2
fi

case "$1" in
  update|configure|build|test)
    STEP=$1
    if [ "$STEP" == "update" ]
    then
      check_and_post_status
    fi
    ;;
  *)
    echo "Usage: $0 (update|configure|build|test)"
    exit 3
    ;;
esac

# Manually source the bash env setup, freeing up $BASH_ENV used by circleci
. /etc/profile >/dev/null

if [ -d /opt/libfabric/1.6.0 ]
then
  export PKG_CONFIG_PATH=$(dirname $(find /opt/libfabric/1.6.0 -name libfabric.pc))
fi

if [ -x /opt/cmake/bin/ctest ]
then
  CTEST=/opt/cmake/bin/ctest
else
  CTEST=ctest
fi

if [ -d /opt/hdf5/1.10.4 ]
then
  export PATH=/opt/hdf5/1.10.4/bin:$PATH
  export LD_LIBRARY_PATH=/opt/hdf5/1.10.4/lib:$LD_LIBRARY_PATH
fi

${CTEST} -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF -Ddashboard_do_${STEP}=TRUE -DCTEST_BUILD_NAME=${CUSTOM_BUILD_NAME}
