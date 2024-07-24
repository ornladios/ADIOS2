#!/bin/sh

set -x

if [ "$(id -u)" -eq 0 ]; then
    # we run as root in CI in docker images, this is OK, but we have to tell XRootD that it's OK.
    XROOTD_USER="daemon"
fi
"$1" -b -l /tmp/xroot.log -R "$XROOTD_USER" -w "$2" -c "$2"/xroot/etc/xrootd/xrootd-ssi.cfg
