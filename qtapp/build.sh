#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

INC=sysroot/include
LIB=sysroot/lib
OUT=build/qtapp

mkdir -p build
zig c++ \
  -target aarch64-linux-gnu.2.29 \
  -std=c++17 -fPIC -O2 \
  -DQT_NO_DEBUG -DQT_CORE_LIB -DQT_GUI_LIB -DQT_WIDGETS_LIB \
  -I "$INC" \
  -I "$INC/QtCore" \
  -I "$INC/QtGui" \
  -I "$INC/QtWidgets" \
  -I "$INC/QtNetwork" \
  -L "$LIB" \
  -Wl,-rpath,/usr/lib \
  src/main.cpp \
  -lQt5Widgets -lQt5Gui -lQt5Network -lQt5Core -lpthread -ldl \
  -o "$OUT"

file "$OUT" || true
ls -lh "$OUT"
