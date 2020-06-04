#!/bin/sh

function usage()
{
  echo "Usage: github-post-status.sh -t <token> -r <repo> -c <sha> -s <state> [-d <description>] [-u <url>] [-x <context>]"
  echo
  echo "Post a commit status to GitHub"
  echo
  echo "Arguments:"
  echo "  -t,--token        GitHub access token"
  echo "  -r,--repo         GitHub repo"
  echo "  -c,--sha          Commit SHA"
  echo "  -s,--state        State"
  echo "  -d,--description  Description"
  echo "  -u,--url          Associated URL"
  echo "  -x,--context      Context"
}

TOKEN=
REPO=
SHA=
STATE=
DESC=
URL=
CONTEXT=
while [ $# -gt 0 ]
do
  case $1
  in
    -t|--token) TOKEN=$2 ;;
    -r|--repo) REPO=$2 ;;
    -c|--sha) SHA=$2 ;;
    -s|--state) STATE=$2 ;;
    -d|--description) DESC="$2" ;;
    -u|--url) URL="$2" ;;
    -x|--context) CONTEXT="$2" ;;
    *)
      echo "Error: Unknown argument: $1"
      exit 1
      ;;
  esac
  shift 2
done

if [ -z "${TOKEN}" ]
then
  echo "Error: No token specified"
  exit 1
fi
if [ -z "${REPO}" ]
then
  echo "Error: No repo specified"
  exit 1
fi
if [ -z "${SHA}" ]
then
  echo "Error: No commit SHA specified"
  exit 1
fi
if [ -z  "${STATE}" ]
then
  echo "Error: No state specified"
  exit 1
fi

JSON_PARAMS="{ \"state\": \"${STATE}\""
if [ -n "${DESC}"} ]
then
  JSON_PARAMS="${JSON_PARAMS}, \"description\": \"${DESC}\""
fi
if [ -n "${URL}"} ]
then
  JSON_PARAMS="${JSON_PARAMS}, \"target_url\": \"${URL}\""
fi
if [ -n "${CONTEXT}"} ]
then
  JSON_PARAMS="${JSON_PARAMS}, \"context\": \"${CONTEXT}\""
fi
JSON_PARAMS="${JSON_PARAMS} }"

AUTH_HEADER="Authorization: token ${TOKEN}"
POST_URL="https://api.github.com/repos/${REPO}/statuses/${SHA}"

HTTP_CODE="$(curl --write-out '%{http_code}' --silent --output /dev/null --show-error -H "${AUTH_HEADER}" -d "${JSON_PARAMS}" -X POST ${POST_URL})"
if [ $? -eq 0 -a "${HTTP_CODE}" = "201" ]
then
  exit 0
fi
exit 2
