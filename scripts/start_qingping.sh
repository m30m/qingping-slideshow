#!/bin/sh
# Hide our qtapp so QingSnow2App becomes visible
PID=$(pgrep -x qtapp)
[ -n "$PID" ] && kill -USR2 "$PID"
