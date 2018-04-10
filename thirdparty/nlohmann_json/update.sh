#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="nlohmann_json"
readonly ownership="JSON For Modern C++ Upstream <robot@adios2>"
readonly subtree="thirdparty/NLohmannJson/json"
#readonly repo="https://github.com/nlohmann/json.git"
#readonly tag="v3.1.2"
readonly repo="https://github.com/chuckatkins/json.git"
readonly tag="misc-cmake-packaging-enhancements"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
