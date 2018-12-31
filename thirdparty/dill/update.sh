#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="dill"
readonly ownership="dill Upstream <robot@adios2>"
readonly subtree="thirdparty/dill/dill"
#readonly repo="https://github.com/GTkorvo/dill.git"
#readonly tag="master"
readonly repo="https://github.com/chuckatkins/dill.git"
readonly tag="misc-cmake-updates"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
