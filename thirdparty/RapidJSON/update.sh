#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="RapidJSON"
readonly ownership="RapidJSON Upstream <robot@adios2>"
readonly subtree="thirdparty/RapidJSON/RapidJSON"
readonly repo="https://github.com/Tencent/rapidjson.git"
readonly tag="master"
readonly shortlog="true"
readonly paths="
  include/
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
