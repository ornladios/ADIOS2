#!/bin/sh

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

echo "Generating config for XRootD with plugin library at $1"
mkdir -p xroot/var/spool
mkdir -p xroot/run/xrootd
mkdir -p xroot/etc/xrootd
BASEDIR="$(pwd)"
{
    # SSI stacked over the default file system: SSI requests go to the ADIOS
    # service; paths under ssi.fspath are ordinary files (byte-range reads of
    # campaign image/text replicas use them).
    echo "xrootd.fslib libXrdSsi.so default";
    echo ""
    echo "all.export /etc nolock r/w";
    echo ""
    echo "oss.statlib -2 libXrdSsi.so";
    echo ""
    echo "ssi.svclib $1";
    echo ""
    echo "ssi.fspath ${BASEDIR}";
    echo "all.export ${BASEDIR} r/o";
    echo ""
} > xroot/etc/xrootd/xrootd-ssi.cfg
