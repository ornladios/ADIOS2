#!/usr/bin/bash

PULLREQUESTS=${CIRCLE_PULL_REQUESTS}   # comma-separated list of pr urls
REPO=${CIRCLE_PROJECT_REPONAME}        # repo name
OWNER=${CIRCLE_PROJECT_USERNAME}       # organization or user the repo lives under
TOKEN=${GITHUB_API_KEY}                # added to env via CircleCI project settings
COMMIT=${CIRCLE_SHA1}                  # sha being tested
COMPAREURL=${CIRCLE_COMPARE_URL}       # CircleCI provides url for comparing this sha with previous

SHASNIP=$(echo $COMMIT | cut -c1-7)    # Grab first 7 characters of the sha to use as link text
CDASHPROJECT="ADIOS"
CDASHURL="https://open.cdash.org/index.php?compare1=61&filtercount=1&field1=revision&project=${CDASHPROJECT}&showfilters=0&limit=100&value1=${COMMIT}&showfeed=0"
COMMENTBODY="View build and test results for change [${SHASNIP}](${COMPAREURL}) on [CDash](${CDASHURL})"
APIBASEURL="https://api.github.com/repos"

get_post_data()
{
  cat <<EOF
{
  "body": "$COMMENTBODY"
}
EOF
}

postBody="$(get_post_data)"

IFS=',' read -ra ADDR <<< "$PULLREQUESTS"
for i in "${ADDR[@]}"; do
    # For each pr url, find the pr number
    if [[ "$i" =~ ADIOS2/pull/([[:digit:]]+) ]];
    then
      prNum=${BASH_REMATCH[1]}
      postUrl="${APIBASEURL}/${OWNER}/${REPO}/issues/${prNum}/comments"

      curl -u "${OWNER}:${TOKEN}" "${postUrl}" -H "Content-Type: application/json" -H "Accept: application/vnd.github.v3+json" -d "${postBody}"
    else
      echo "Unable to find PR number in ${i}"
    fi
done

