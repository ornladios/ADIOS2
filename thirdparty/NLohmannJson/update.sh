#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="NLohmannJson"
readonly ownership="JSON For Modern C++ Upstream <robot@adios2>"
readonly subtree="thirdparty/NLohmannJson/json"
readonly repo="https://github.com/nlohmann/json.git"
readonly tag="v3.1.2"
readonly shortlog="true"
readonly paths="
LICENSE.MIT
single_include/nlohmann/json.hpp
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
