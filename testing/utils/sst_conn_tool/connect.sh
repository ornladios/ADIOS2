#!/bin/sh

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Start sst_conn_tool as a listener, wait until it publishes its contact file,
# then connect to it.
# Usage: connect.sh <path-to-sst_conn_tool>

tool=$1
log=sst_conn_tool_listen.log
contact=SstConnToolTemp.sst

rm -f "${contact}"

"${tool}" > "${log}" 2>&1 &
pid=$!

tries=0
until [ -f "${contact}" ]; do
  tries=$((tries + 1))
  if [ "${tries}" -gt 300 ] || ! kill -0 "${pid}" 2> /dev/null; then
    echo "listener did not become ready"
    cat "${log}"
    kill "${pid}" 2> /dev/null
    exit 1
  fi
  sleep 0.1
done

"${tool}" --connect || { kill "${pid}" 2> /dev/null; exit 1; }
wait "${pid}"
