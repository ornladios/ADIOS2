#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="inih"
readonly ownership="inih Upstream <robot@adios2>"
readonly subtree="thirdparty/inih/inih"
readonly repo="https://github.com/benhoyt/inih"
readonly tag="master"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
