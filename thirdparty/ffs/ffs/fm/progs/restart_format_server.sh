#!/bin/bash
# Wrapper script to properly restart format_server
# Works around PID reuse bug in format_server's built-in detection

if ! pgrep -x format_server > /dev/null; then
    rm -f /tmp/format_server_pid
    /home/ge10/bin/format_server -quiet
fi
