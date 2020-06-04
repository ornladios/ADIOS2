#!/bin/bash --login

set -x
set -e

if [ "${GITHUB_EVENT_NAME}" = "pull_request" ]
then
  GH_PR_NUMBER=$(expr "${GITHUB_REF}" : 'refs/pull/\([^/]*\)')
  ORIG_VERSION_SUFFIX="pr${GH_PR_NUMBER}"
  CI_BUILD_NAME="pr${GH_PR_NUMBER}_${GITHUB_HEAD_REF}_${GH_YML_JOBNAME}"
else
  ORIG_VERSION_SUFFIX="g$(git show --format=%ad --date=format:%Y.%m.%d --quiet HEAD)"
  CI_BUILD_NAME="${GITHUB_REF#refs/heads/}_${GH_YML_JOBNAME}"
fi
ORIG_VERSION="$(git grep -h setup_version CMakeLists.txt | sed -E 's/^setup_version\((.*)\)$/\1/')+${ORIG_VERSION_SUFFIX}"
DEBIAN_VERSION="${ORIG_VERSION}-1~"

export DEB_CTEST_OPTIONS="site=\"GitHub Actions\" build=\"${CI_BUILD_NAME}\" model=Experimental track=\"Continuous Integration\" catchfailed submit update"

STEP=$1
case ${STEP} in
  prepare)
    cp -r scripts/ci/package/ci-${GH_YML_JOBNAME}/debian debian
    if [ "${GITHUB_EVENT_NAME}" = "pull_request" ]
    then
      DESCRIPTION="GitHub PR ${GH_PR_NUMBER}"
      export DEBFULLNAME="$(git show --format=%an --quiet HEAD)"
      export DEBEMAIL="$(git show --format=%ae --quiet HEAD)"
    else
      DESCRIPTION="Nightly build"
      export DEBFULLNAME="Kitware Robot"
      export DEBEMAIL="kwrobot@kitware.com"
    fi
    dch --create -v "${DEBIAN_VERSION}" --package adios2 -D UNRELEASED "${DESCRIPTION}"
    git archive -o "../adios2_${ORIG_VERSION}.orig.tar.gz" --format tar.gz --prefix "adios2_${ORIG_VERSION}/" HEAD
  ;;

  build)
    debuild --set-envvar OMPI_ALLOW_RUN_AS_ROOT=1 --set-envvar OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1 -us -uc
  ;;
esac
