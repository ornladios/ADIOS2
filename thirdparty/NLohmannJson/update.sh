#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="NLohmannJson"
readonly ownership="JSON For Modern C++ Upstream <robot@adios2>"
readonly subtree="thirdparty/NLohmannJson/json"
readonly repo="https://github.com/nlohmann/json.git"
readonly tag="v2.1.1"
readonly shortlog="true"
readonly paths="
LICENSE.MIT
src/json.hpp
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
