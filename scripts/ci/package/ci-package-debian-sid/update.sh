#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="adios2-debian"
readonly ownership="Kitware Debian Maintainers <debian@kitware.com>"
readonly subtree="scripts/ci/package/ci-package-debian-sid/debian"
readonly repo="https://gitlab.kitware.com/debian/adios2.git"
readonly tag="master"
readonly shortlog="true"

extract_source () {
    git archive --worktree-attributes HEAD -- debian | \
        tar -C "$extractdir" -x --transform "s/^debian/$name-reduced/" --exclude debian/changelog
}

. "${BASH_SOURCE%/*}/../../../../thirdparty/update-common.sh"
