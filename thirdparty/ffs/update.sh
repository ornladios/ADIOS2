#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="ffs"
readonly ownership="ffs Upstream <robot@adios2>"
readonly subtree="thirdparty/ffs/ffs"
readonly repo="https://github.com/GTkorvo/ffs.git"
readonly tag="master"
readonly shortlog="true"
readonly exact_tree_match="false"
readonly paths="
"

extract_source () {
    git_archive
    pushd "$extractdir/$name-reduced"
    rm -rf ./scripts/
    rm -rf .github/
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
