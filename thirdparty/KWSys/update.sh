#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWSys"
readonly ownership="KWSys Upstream <kwrobot@kitware.com>"
readonly subtree="thirdparty/KWSys/adios2sys"
readonly repo="https://gitlab.kitware.com/utils/kwsys.git"
readonly tag="master"

readonly shortlog="true"
readonly exact_tree_match="false"
readonly paths="
"

git_archive_no_attributes () {
    find . -name gitattributes | xargs git rm -f
    git archive --prefix="$name-reduced/" HEAD -- $paths | \
        tar -C "$extractdir" -x
}

extract_source () {
    git_archive_no_attributes
    pushd "$extractdir/$name-reduced"
    patch -p4 --no-backup-if-mismatch < "${toplevel_dir}/thirdparty/KWSys/patches/0001-wrap-TARGET_OBJECTS-in-BUILD_INTERFACE.patch"
    popd
}

. "${BASH_SOURCE%/*}/../update-common.sh"
