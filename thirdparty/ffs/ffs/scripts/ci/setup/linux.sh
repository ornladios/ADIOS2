#!/bin/bash

case ${GH_YML_JOBNAME} in
  centos7*) PKG_CMD=yum ;;
  centos8*|alma8*) PKG_CMD=dnf ;;
  centos9*|alma9*) PKG_CMD=dnf ;;
  centos10*|alma10*) PKG_CMD=dnf ;;
  ubuntu*)  PKG_CMD=apt-get ;;
esac

########################################
# Baseline dependencies
########################################
case ${GH_YML_JOBNAME} in
  centos*|alma*) PKGS="epel-release make curl perl bison flex" ;;
  ubuntu*)
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    PKGS="software-properties-common make curl perl bison flex"
    ;;
esac
${PKG_CMD} install -y ${PKGS}

########################################
# Git
########################################
case ${GH_YML_JOBNAME} in
  centos7*)
    curl -L https://copr.fedorainfracloud.org/coprs/g/git-maint/git/repo/epel-7/group_git-maint-git-epel-7.repo > /etc/yum.repos.d/group_git-maint-git-epel-7.repo
    ;;
  centos8*|alma8*)
    curl -L https://copr.fedorainfracloud.org/coprs/g/git-maint/git/repo/epel-8/group_git-maint-git-epel-8.repo > /etc/yum.repos.d/group_git-maint-git-epel-8.repo
    ;;
  centos9*|alma9*)
    curl -L https://copr.fedorainfracloud.org/coprs/g/git-maint/git/repo/epel-9/group_git-maint-git-epel-9.repo > /etc/yum.repos.d/group_git-maint-git-epel-9.repo
    ;;
  centos10*|alma10*)
    curl -L https://copr.fedorainfracloud.org/coprs/g/git-maint/git/repo/epel-10/group_git-maint-git-epel-10.repo > /etc/yum.repos.d/group_git-maint-git-epel-10.repo
    ;;
  ubuntu*)
    export DEBIAN_FRONTEND=noninteractive
    add-apt-repository ppa:git-core/ppa -y
    apt-get update
    ;;
esac
${PKG_CMD} install -y git

########################################
# Compilers
########################################
case ${GH_YML_JOBNAME} in
  centos*|alma*)   PKGS="gcc gcc-c++" ;;
  ubuntu*)   PKGS="gcc g++" ;;
esac
if [ "${GH_YML_JOBNAME##*-}" = "clang" ]
then
  PKGS="clang ${PKGS}"
fi
echo "LINUX.SH INSTALLING PACKAGES ${PKGS}"
${PKG_CMD} install -y ${PKGS}


case ${GH_YML_JOBNAME} in
  *-clang) export CC=clang CXX=clang++ ;;
  *-gcc)   export CC=gcc CXX=g++ ;;
  *-nvhpc) export CC=nvc CXX=nvc++ ;;
esac

echo "LINUX.SH BEFORE CMAKE"
########################################
# CMake
########################################
FILENAME=$(curl https://cmake.org/files/LatestRelease/cmake-latest-files-v1.json 2>/dev/null | grep 'cmake.*sh' | sed -n 's|.*"\(cmake.*linux-x86_64.sh\).*|\1|p')
VERSION=$(echo ${FILENAME} | sed 's|cmake-\([^\-]*\).*|\1|')
curl -L https://github.com/Kitware/CMake/releases/download/v${VERSION}/${FILENAME} > cmake.sh
chmod +x cmake.sh
./cmake.sh --skip-license --exclude-subdir --prefix=/usr/local
rm -f cmake.sh
echo "FINISH LINUX.SH"
