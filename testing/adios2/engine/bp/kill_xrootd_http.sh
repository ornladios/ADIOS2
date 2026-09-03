#!/bin/bash

# SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
#
# SPDX-License-Identifier: Apache-2.0

# Kill XRootD HTTP server

PID_FILE="${1:-/tmp/xrootd-http.pid}"
SHUTDOWN_WAIT_SECONDS=2

process_is_running()
{
    if ! kill -0 "$1" 2>/dev/null; then
        return 1
    fi

    PROCESS_STATE=$(ps -p "$1" -o stat= 2>/dev/null)
    case "${PROCESS_STATE}" in
        ''|Z*) return 1 ;;
        *) return 0 ;;
    esac
}

if [ ! -f "${PID_FILE}" ]; then
    echo "XRootD HTTP PID file does not exist: ${PID_FILE}"
    exit 0
fi

PID=$(cat "${PID_FILE}")
case "${PID}" in
    ''|*[!0-9]*)
        echo "ERROR: Invalid XRootD HTTP PID in ${PID_FILE}: ${PID}"
        exit 1
        ;;
esac

if ! process_is_running "${PID}"; then
    echo "Removing stale XRootD HTTP PID file for process ${PID}"
    rm -f "${PID_FILE}"
    exit 0
fi

COMMAND=$(ps -p "${PID}" -o comm= 2>/dev/null)
case "${COMMAND##*/}" in
    xrootd*) ;;
    *)
        echo "ERROR: Refusing to stop non-XRootD process ${PID}: ${COMMAND}"
        exit 1
        ;;
esac

echo "Stopping XRootD HTTP server process ${PID}"
if ! kill -TERM "${PID}"; then
    echo "ERROR: Failed to signal XRootD HTTP server process ${PID}"
    exit 1
fi

for ((i = 0; i < SHUTDOWN_WAIT_SECONDS * 10; ++i)); do
    if ! process_is_running "${PID}"; then
        rm -f "${PID_FILE}"
        echo "XRootD HTTP server process ${PID} stopped"
        exit 0
    fi
    sleep 0.1
done

echo "XRootD HTTP server process ${PID} did not stop within ${SHUTDOWN_WAIT_SECONDS}s; sending SIGKILL"
if ! kill -KILL "${PID}"; then
    echo "ERROR: Failed to kill XRootD HTTP server process ${PID}"
    exit 1
fi

for ((i = 0; i < 50; ++i)); do
    if ! process_is_running "${PID}"; then
        rm -f "${PID_FILE}"
        echo "XRootD HTTP server process ${PID} killed"
        exit 0
    fi
    sleep 0.1
done

echo "ERROR: XRootD HTTP server process ${PID} is still present after SIGKILL"
exit 1
