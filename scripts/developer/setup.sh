#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

function test_cmd {
  if ! $1 ;
  then
    echo "Error: $2";
    exit $3
  fi
}

cd "${BASH_SOURCE%/*}/../.."

test_cmd scripts/developer/git/setup-remotes \
  "Failed to setup upstream remotes " 2

test_cmd scripts/developer/git/setup-aliases \
  "Failed to setup git aliases" 2

test_cmd scripts/developer/git/setup-hooks \
  "Failed to setup git hooks" 1
git config hooks.clang-format false
