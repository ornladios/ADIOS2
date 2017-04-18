#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="KWSys"
readonly ownership="KWSys Upstream <kwrobot@kitware.com>"
readonly subtree="thirdparty/KWSys/adios2sys"
#readonly repo="https://gitlab.kitware.com/utils/kwsys.git"
#readonly tag="0c4e58ec"

# This commit contains a patch so suppress noisy warnings.  Use 'master' off
# the main repo instead once its been merged
readonly repo="https://gitlab.kitware.com/chuck.atkins/kwsys.git"
readonly tag="0c4e58ec"

readonly shortlog="true"
readonly paths="
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
