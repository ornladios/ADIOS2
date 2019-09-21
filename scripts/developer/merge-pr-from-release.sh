#!/bin/bash

if [ $# -ne 1 ]
then
  echo "Usage: $0 [pull request number]"
  exit 1
fi

git fetch upstream

if [ "$(echo $(git status -s -b))" != "## master...upstream/master" ]
then
  echo "Error: Must be called from the local up-to-date master branch"
  exit 2
fi

PR=$1
PR_MERGE_COMMIT=$(git log --oneline --merges --first-parent --no-decorate upstream/release | awk -v PRMATCH="#${PR}" '$5==PRMATCH {print $1}')
if [ -z "${PR_MERGE_COMMIT}" ]
then
  echo "Error: Unable to find pull request #${PR}"
  exit 3
fi

PR_BASE_COMMIT=$(git show -s --oneline ${PR_MERGE_COMMIT}^2 | awk '{print $1}')

if ! git merge --no-ff --no-commit ${PR_BASE_COMMIT} 2>/dev/null
then
  echo "Error: merge failed"
  git reset --hard
  exit 4
fi

if ! git commit -C ${PR_MERGE_COMMIT}
then
  echo "Error: merge failed"
  git reset --hard
  exit 5
fi

git merge --no-ff --no-commit ${PR_MERGE_COMMIT} 2>/dev/null
git commit -m "Merge release"
