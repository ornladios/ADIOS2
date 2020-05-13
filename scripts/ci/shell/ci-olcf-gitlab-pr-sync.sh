#!/usr/bin/env bash

set -e

if [ -z "${CI_SOURCE_DIR}" ]
then
  echo "Error: CI_SOURCE_DIR is empty"
  exit 1
fi
if [ -z "${OLCF_GITLAB_SSH_KEY_BASE64}" ]
then
  echo "Error: OLCF_GITLAB_SSH_KEY_BASE64 is empty"
  exit 1
fi

TMPKEY=$(mktemp)
echo "${OLCF_GITLAB_SSH_KEY_BASE64}" | base64 -d > ${TMPKEY}

# Setup the olcf remote
cd ${CI_SOURCE_DIR}
git remote add olcf git@code.ornl.gov:ecpcitest/adios2.git

export GIT_SSH_COMMAND="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no -i ${TMPKEY} -F /dev/null"
git fetch -p olcf

# Retrieve open PRs
OPEN_PRS="$(curl 'https://api.github.com/repos/ornladios/adios2/pulls?state=open&sort=created' 2>/dev/null | jq '.[] | {number: .number}' | sed -n 's| *\"number\" *: *\([0-9]\+\)|\1|p')"

# Retrieve the list of sync'd PRs 
SYNCD_PRS="$(git ls-remote olcf github/pr* | sed -n 's|.*/github/pr\([0-9]\+\)|\1|p')"

# Determine any closed PRs that are currently sync'd
SYNCD_CLOSED_PRS=""
if [ -n "${SYNCD_PRS}" ]
then
  for PR in ${SYNCD_PRS}
  do
    if [ -n "${OPEN_PRS}" ]
    then
      IS_OPEN=0
      for OPEN_PR in ${OPEN_PRS}
      do
        if [ ${PR} -eq ${OPEN_PR} ]
        then
          IS_OPEN=1
          break
        fi
      done
      if [ ${IS_OPEN} -eq 0 ]
      then
        SYNCD_CLOSED_PRS="${SYNCD_CLOSED_PRS} ${PR}"
      fi
    fi
  done
fi

# Delete any sync'd PRs
if [ -n "${SYNCD_CLOSED_PRS}" ]
then
  for PR in ${SYNCD_CLOSED_PRS}
  do
    echo "Removing ${PR}"
    git push -f olcf :github/pr${PR}
  done
fi

# Sync open PRs to OLCF
if [ -n "${OPEN_PRS}" ]
then
  for PR in ${OPEN_PRS}
  do
    echo "Syncing ${PR}"
    git fetch origin +refs/pull/${PR}/head:refs/remotes/origin/github/pr${PR}
    git branch github/pr${PR} origin/github/pr${PR}
    git push -f olcf github/pr${PR}:github/pr${PR}
  done
fi

exit 0
