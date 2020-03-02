#!/bin/bash

set -e

target_dir="${HOME}/cmake/${CI_ENVIRONMENT}"
if [ -d "$target_dir" ] ; then
    echo "CMake (${CI_ENVIRONMENT}) already downloaded"
    exit 0
fi

case ${CI_ENVIRONMENT} in
  x86_64)
    url="$( curl https://cmake.org/files/dev/ |
            sed -n '/Linux-x86_64.tar.gz/s/.*>\(cmake[^<]*\)<.*/\1/p' |
            sort |
            tail -1 )"

    url="https://cmake.org/files/dev/$url"
    ;;

  p9)
    url='https://data.kitware.com/api/v1'
    url="$url/item/5e6153b0af2e2eed35022bc8/download"
    ;;

  *)
    echo "Unrecognized CI environment: \"${CI_ENVIRONMENT}\""
    exit 1
    ;;
esac

tmpdir="$( mktemp -d )"
trap "rm -rf \"$tmpdir\"" EXIT INT TERM QUIT
cd "$tmpdir"

curl -o - "$url" | tar xzf -
mkdir -p "$( dirname "$target_dir" )"
mv * "$target_dir"
