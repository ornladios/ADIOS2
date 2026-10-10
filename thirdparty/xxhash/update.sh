#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="xxhash"
readonly ownership="xxHash Upstream <robot@adios2>"
readonly subtree="thirdparty/xxhash/xxhash_wrapper"
readonly repo="https://github.com/Cyan4973/xxHash.git"
readonly tag="v0.8.3"
readonly shortlog="true"
readonly exact_tree_match="false"
readonly paths="
LICENSE
xxhash.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
