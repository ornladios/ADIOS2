#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="enet"
readonly ownership="enet Upstream <robot@adios2>"
readonly subtree="thirdparty/zpl-enet/zpl-enet"
readonly repo="https://github.com/eisenhauer/zpl-enet.git"
readonly tag="master"
readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
