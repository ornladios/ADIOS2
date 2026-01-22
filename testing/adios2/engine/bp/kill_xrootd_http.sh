#!/bin/bash
# Kill XRootD HTTP server
kill -2 "$(cat /tmp/xrootd-http.pid)"
#rm /tmp/xrootd-http.pid
exit 0
