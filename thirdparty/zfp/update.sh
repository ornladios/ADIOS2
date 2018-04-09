#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="zfp"
readonly ownership="zfp Upstream <robot@adios2>"
readonly subtree="thirdparty/zfp/zfp"
readonly repo="https://github.com/chuckatkins/zfp.git"
readonly tag="misc-cmake-updates"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
