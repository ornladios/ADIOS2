#!/usr/bin/env bash

test_cmd {
  if ! $1 ;
  then
    echo "Error: $2";
    exit $3
  fi
}

cd "${BASH_SOURCE%/*}"

test_cmd scripts/git/setup-alias \
  "Failed to setup git aliases" 2

test_cmd scripts/git/setup-hooks \
  "Failed to setup git hooks" 1
git config hooks.clang-format true
