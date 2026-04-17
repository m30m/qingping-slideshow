#!/bin/sh
URL="${QTAPP_URL}"
INTERVAL="${QTAPP_INTERVAL:-5}"

if [ -z "$URL" ]; then
    echo "QTAPP_URL not set" >&2
    exit 1
fi

PID=$(pgrep -x qtapp)
if [ -n "$PID" ]; then
    kill -USR1 "$PID"
else
    export QT_QPA_PLATFORM=wayland-egl
    export WAYLAND_DISPLAY=wayland-0
    export XDG_RUNTIME_DIR=/tmp/.xdg
    /userdata/qtapp "$URL" "$INTERVAL" &
fi
