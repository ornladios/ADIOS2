#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="EVPath"
readonly ownership="EVPath Upstream <robot@adios2>"
readonly subtree="thirdparty/EVPath/EVPath"
#readonly repo="https://github.com/GTkorvo/EVPath.git"
#readonly tag="master"
readonly repo="https://github.com/chuckatkins/EVPath.git"
readonly tag="fix-libfabric-to-use-pkgconfig"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
