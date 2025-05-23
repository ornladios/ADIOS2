#!/bin/bash
kill -2 "$(cat /tmp/xrootd.pid)"
#rm /tmp/xrootd.pid
exit 0
