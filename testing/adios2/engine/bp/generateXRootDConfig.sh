#!/bin/sh
echo "First arg is $1"
mkdir -p xroot/var/spool
mkdir -p xroot/run/xrootd
mkdir -p xroot/etc/xrootd
{
    echo "xrootd.fslib libXrdSsi.so";
    echo ""
    echo "all.export ${PWD}/xroot/data nolock r/w";
    echo ""
    echo "oss.statlib -2 libXrdSsi.so";
    echo ""
    echo "ssi.svclib $1";
    echo ""
} > xroot/etc/xrootd/xrootd-ssi.cfg
