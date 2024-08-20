#!/bin/sh
echo "Generating config for XRootD with plugin library at $1"
mkdir -p xroot/var/spool
mkdir -p xroot/run/xrootd
mkdir -p xroot/etc/xrootd
{
    echo "xrootd.fslib libXrdSsi.so";
    echo ""
    echo "all.export /etc nolock r/w";
    echo ""
    echo "oss.statlib -2 libXrdSsi.so";
    echo ""
    echo "ssi.svclib $1";
    echo ""
} > xroot/etc/xrootd/xrootd-ssi.cfg
