#!/bin/bash

export CI_ROOT_DIR="${GITHUB_WORKSPACE//\\//}/.."
export CI_SOURCE_DIR="${GITHUB_WORKSPACE//\\//}"
export CI_DEP_DIR="${CI_ROOT_DIR}/dependencies"
export CI_BIN_DIR="${CI_ROOT_DIR}/build"

export CMAKE_PREFIX_PATH=${CI_DEP_DIR}/install
export PATH=${CI_DEP_DIR}/tools/bin:${CI_DEP_DIR}/install/bin:${PATH}
case "$(uname -s)" in
  Linux)
    export LD_LIBRARY_PATH=${CI_DEP_DIR}/install/lib:${LD_LIBRARY_PATH}
    ;;
  Darwin)
    export DYLD_LIBRARY_PATH=${CI_DEP_DIR}/install/lib:${DYLD_LIBRARY_PATH}
    ;;
esac


mkdir -p ${CI_BIN_DIR}
cd ${CI_BIN_DIR}

case "$1" in
  configure)
    cmake -GNinja -DCMAKE_INSTALL_PREFIX=${CI_ROOT_DIR}/install ${CI_SOURCE_DIR}
    ;;
  build)
    ninja
    ;;
  test)
    if [ "$(uname -s)" = "Darwin" ]
    then
      # Disable the firewall
      sudo /usr/libexec/ApplicationFirewall/socketfilterfw --setglobalstate off 

      # Force the use of the loopback interface
      #export CM_IP="127.0.0.1"
      export CM_HOSTNAME="localhost"
      CTEST_EXCLUDES="mtests_non_blocking_bulk"
    fi
    ctest --timeout 300 -j2 -VV -E "${CTEST_EXCLUDES}"
    ;;
  install)
    ninja install
    ;;
esac
