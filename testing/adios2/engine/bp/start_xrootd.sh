#!/bin/sh
if [ "$(id -u)" -eq 0 ]; then
    # we run as root in CI in docker images, this is OK, but we have to tell XRootD that it's OK.
    ROOT_ARGS="-R sshd"
fi
"$1" -b -l /tmp/xroot.log "$ROOT_ARGS" -w "$2" -c "$2"/xroot/etc/xrootd/xrootd-ssi.cfg
