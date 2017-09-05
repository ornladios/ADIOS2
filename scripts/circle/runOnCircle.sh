#!/bin/bash

API_BASE="https://api.github.com/repos/ornladios/adios2"
USER=${STATUS_ROBOT_NAME}
TOKEN=${STATUS_ROBOT_KEY}
COMMIT=${CIRCLE_SHA1}
CDASH_STATUS_CONTEXT="cdash"

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
  PYTHON_SCRIPT="${SOURCE_DIR}/scripts/circle/findStatus.py"
  curl -u "${STATUS_ROBOT_NAME}:${STATUS_ROBOT_KEY}" "${API_BASE}/commits/${COMMIT}/statuses" | python ${PYTHON_SCRIPT} --context ${CDASH_STATUS_CONTEXT}
  if [ $? -ne 0 ]
  then
    echo "Need to post a status for context ${CDASH_STATUS_CONTEXT}"
    postBody="$(build_status_body)"
    postUrl="${API_BASE}/statuses/${COMMIT}"
    curl -u "${OWNER}:${TOKEN}" "${postUrl}" -H "Content-Type: application/json" -H "Accept: application/vnd.github.v3+json" -d "${postBody}"
  fi
}

get_real_branch_name() {
  APIURL="${API_BASE}/pulls/${CIRCLE_PR_NUMBER}"
  RESULT=`curl -s ${APIURL} | python -c "import sys, json; print(json.load(sys.stdin)['head']['ref'])" 2> /dev/null`

  if [ $? -eq 0 ]
  then
    REALBRANCH=$RESULT
  else
    REALBRANCH=$CIRCLE_BRANCH
  fi
}

check_var() {
  if [ -z "$1" ]
  then
    echo "Error: The $1 environment variable is undefined"
    exit 1
  fi
}

check_var CIRCLE_WORKING_DIRECTORY
check_var CIRCLE_BRANCH
check_var CIRCLE_JOB
check_var CIRCLE_BUILD_NUM
check_var CIRCLE_PR_NUMBER

if [ ! "${CUSTOM_BUILD_NAME}" ]
then
  get_real_branch_name

  LINETOSAVE="export CUSTOM_BUILD_NAME=${REALBRANCH}_${CIRCLE_BUILD_NUM}_${CIRCLE_JOB}"

  # Set the custom build name for this step
  eval $LINETOSAVE

  # Also make sure it will get set for the following steps
  echo "${LINETOSAVE}" >> $BASH_ENV
fi

SOURCE_DIR=${CIRCLE_WORKING_DIRECTORY}/source
CTEST_SCRIPT="${SOURCE_DIR}/scripts/circle/circle_${CIRCLE_JOB}.cmake"

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

/opt/cmake/3.6.0/bin/ctest -VV -S ${CTEST_SCRIPT} -Ddashboard_full=OFF -Ddashboard_do_${STEP}=TRUE -DCTEST_BUILD_NAME=${CUSTOM_BUILD_NAME}
