#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="nanobind"
readonly ownership="nanobind Upstream <robot@adios2>"
readonly subtree="thirdparty/nanobind/nanobind"
readonly repo="https://github.com/wjakob/nanobind.git"
readonly tag="v2.12.0"
readonly shortlog="true"
readonly exact_tree_match="false"

extract_source () {
    # nanobind uses git submodules (robin_map), so use git_archive_all
    # to capture the complete source tree including submodules.
    git_archive_all
    pushd "$extractdir/$name-reduced"
    rm -rf .github/
    rm -rf docs/
    rm -rf tests/
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
