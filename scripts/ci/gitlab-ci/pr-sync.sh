#!/usr/bin/env bash

set -e

if [ -z "${SSH_KEY_BASE64}" ]
then
  echo "Error: SSH_KEY_BASE64 is empty"
  exit 1
fi

git --version

export GIT_SSH_COMMAND="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no"

# Start the ssh agent
eval $(ssh-agent -s)

# Add the necessary key
echo "${SSH_KEY_BASE64}" | base64 -d | tr -d '\r' | ssh-add -

# Setup push access
git config remote.origin.pushurl $(echo ${CI_REPOSITORY_URL} | sed 's|.*@\([^\/]*\)\/|git@\1:|')

# Setup the github upstream remote
if git remote | grep -q upstream
then
  git remote rm upstream
fi
git remote add upstream https://github.com/ornladios/ADIOS2.git
git fetch -p upstream

# Retrieve open PRs
OPEN_PR_BRANCHES="$(python3 -c 'from github import Github; print(" ".join(["pr%d_%s" % (pr.number, pr.head.ref) for pr in Github().get_repo("ornladios/ADIOS2").get_pulls(state="open")]))')"
echo "Open PRs:"
for PR in ${OPEN_PR_BRANCHES}
do
  echo "  ${PR}"
done

# Retrieve sync'd PRs 
SYNCD_PR_BRANCHES="$(echo $(git ls-remote origin github/pr* | sed -n 's|.*/github/\(pr[0-9].*\)|\1|p'))"
echo "Syncd PRs:"
for PR in ${SYNCD_PR_BRANCHES}
do
  echo "  ${PR}"
done

# Determine any closed PRs that are currently sync'd
SYNCD_CLOSED_PR_BRANCHES=""
if [ -n "${SYNCD_PR_BRANCHES}" ]
then
  for SPR in ${SYNCD_PR_BRANCHES}
  do
    if [ -n "${OPEN_PR_BRANCHES}" ]
    then
      IS_OPEN=0
      for OPR in ${OPEN_PR_BRANCHES}
      do
        if [ "${SPR}" = "${OPR}" ]
        then
          IS_OPEN=1
          break
        fi
      done
      if [ ${IS_OPEN} -eq 0 ]
      then
        SYNCD_CLOSED_PR_BRANCHES="${SYNCD_CLOSED_PR_BRANCHES} ${SPR}"
      fi
    fi
  done
fi
echo "Syncd Closed PRs:"
for PR in ${SYNCD_CLOSED_PR_BRANCHES}
do
  echo "  ${PR}"
done

# Delete any sync'd PRs
if [ -n "${SYNCD_CLOSED_PR_BRANCHES}" ]
then
  CLOSED_REFSPECS=""
  for PR in ${SYNCD_CLOSED_PR_BRANCHES}
  do
    echo "Adding respec for closed ${PR}"
    CLOSED_REFSPECS="${CLOSED_REFSPECS} :github/${PR}"
  done

  echo "Removing closed PRs"
  git push -f origin ${CLOSED_REFSPECS}
fi

# Sync open PRs to OLCF
if [ -n "${OPEN_PR_BRANCHES}" ]
then
  FETCH_REFSPECS=""
  PUSH_REFSPECS=""
  for PR in ${OPEN_PR_BRANCHES}
  do
    PR_NUM=$(expr "${PR}" : 'pr\([0-9]\+\)')
    echo "Adding refspecs for ${PR}"
    FETCH_REFSPECS="${FETCH_REFSPECS} +refs/pull/${PR_NUM}/head:refs/remotes/upstream/github/${PR}"
    PUSH_REFSPECS="${PUSH_REFSPECS} github/${PR}:github/${PR}"
  done

  echo "Fetching upstream refs for open PRs"
  git fetch upstream ${FETCH_REFSPECS}

  for PR in ${OPEN_PR_BRANCHES}
  do
    echo "Building local branch for ${PR}"
    git branch github/${PR} upstream/github/${PR}
  done

  echo "Pushing branches for open PRs"
  git push -f origin ${PUSH_REFSPECS}
fi
