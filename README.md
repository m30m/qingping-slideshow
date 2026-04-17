# Qingping Air Monitor 2 — Custom Apps

Custom Qt5 apps for the Qingping Air Monitor 2 (Rockchip PX30, Buildroot Linux, Qt 5.14.2).

## Device info

- **SoC:** Rockchip PX30, quad-core Cortex-A35 @ 1.5 GHz
- **RAM:** 465 MB
- **GPU:** Mali-G31 (Bifrost)
- **Display:** 720×720
- **OS:** Buildroot 2018.02-rc3, kernel 4.19, glibc 2.29
- **Qt:** 5.14.2 with Wayland + EGLFS platform plugins
- **Compositor:** Weston (DRM backend) — started by the patched watchdog on boot

## Prerequisites

- `adb` — was in `~/Downloads/platform-tools/` last time
- `zig` — `brew install zig` (used as cross-compiler for aarch64-linux-gnu)
- `aqtinstall` — `pip3 install aqtinstall` (to download Qt 5.14.2 headers)

## Project layout

```
qtapp/
  src/main.cpp        — slideshow app source
  build.sh            — cross-compile script (zig c++ → aarch64 ELF)
  build/qtapp         — compiled binary (gitignored)
  sysroot/            — (gitignored, see setup instructions below)
    include/          — Qt 5.14.2 headers (from aqtinstall)
    lib/              — Qt .so files pulled from device
cloud.png               — panel icon for QingSnow2App launcher (pushed to /userdata/cloud.png)
scripts/
  start_qtapp.sh      — weston launcher: starts qtapp or brings it to front (SIGUSR1)
  start_qingping.sh   — weston launcher: hides qtapp (SIGUSR2) to reveal QingSnow2App
  watchdog.sh         — patched watchdog (ensures weston + wayland)
  watchdog.sh.bak     — original vendor watchdog.sh from /qingping/bin/ (for reference)
  weston.ini          — custom weston config with launcher icons
```

## Sysroot setup (one-time)

The sysroot is gitignored because it contains large binary files. Recreate it on a fresh clone:

### 1. Download Qt 5.14.2 headers

```bash
aqt install-qt linux desktop 5.14.2 gcc_64 -O /tmp/qt5
cp -R /tmp/qt5/5.14.2/gcc_64/include qtapp/sysroot/include
rm -rf /tmp/qt5
```

### 2. Pull Qt .so files from the device

Connect the device via USB, then:

```bash
mkdir -p qtapp/sysroot/lib
adb pull /usr/lib/libQt5Core.so.5    qtapp/sysroot/lib/
adb pull /usr/lib/libQt5Gui.so.5     qtapp/sysroot/lib/
adb pull /usr/lib/libQt5Widgets.so.5 qtapp/sysroot/lib/
adb pull /usr/lib/libQt5Network.so.5 qtapp/sysroot/lib/
adb pull /usr/lib/libQt5DBus.so.5    qtapp/sysroot/lib/
```

### 3. Create unversioned symlinks for the linker

```bash
cd qtapp/sysroot/lib
for f in libQt5*.so.5; do ln -sf "$f" "${f%.5}"; done
```

## Building

```bash
cd qtapp
./build.sh
```

This cross-compiles `src/main.cpp` to `build/qtapp` (aarch64 ELF, dynamically linked against on-device Qt 5.14.2).

## Deploying

```bash
adb push qtapp/build/qtapp /userdata/qtapp
adb push scripts/start_qtapp.sh /userdata/start_qtapp.sh
adb push scripts/start_qingping.sh /userdata/start_qingping.sh
adb push scripts/watchdog.sh /qingping/bin/watchdog.sh
adb push scripts/weston.ini /etc/xdg/weston/weston.ini
adb push cloud.png /userdata/cloud.png
adb shell "chmod +x /userdata/qtapp /userdata/start_qtapp.sh /userdata/start_qingping.sh /qingping/bin/watchdog.sh"
```

## Running

### Weston startup

Originally the device boots QingSnow2App directly on EGLFS (no compositor). The patched
`watchdog.sh` changes this: it starts Weston automatically if it's not running, then launches
QingSnow2App as a Wayland client instead of EGLFS. This allows multiple apps to coexist.

To deploy the patched watchdog:

```bash
adb push scripts/watchdog.sh /qingping/bin/watchdog.sh
adb shell "chmod +x /qingping/bin/watchdog.sh"
```

The watchdog checks every 5 seconds. If weston is not running, it creates the XDG runtime dir,
starts `weston --tty=2 --idle-time=0`, and waits 3 seconds before launching QingSnow2App with
`-platform wayland-egl`.

To start Weston manually (e.g. after killing it):

```bash
adb shell "mkdir -p /tmp/.xdg && chmod 0700 /tmp/.xdg"
adb shell "export XDG_RUNTIME_DIR=/tmp/.xdg; setsid weston --tty=2 --idle-time=0 &"
```

Reference: https://blog.29b.net/dispatches/cgs2_decloud/

### Running the slideshow app

```bash
adb shell "export QT_QPA_PLATFORM=wayland-egl WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/tmp/.xdg; \
  /userdata/qtapp 'https://example.com/image-endpoint' 15 &"
```

Arguments:
- `$1` — URL to fetch a new image from (required)
- `$2` — refresh interval in seconds (default: 5)

### Using the launcher scripts

The scripts are wired to weston panel icons via `weston.ini`:

- **Window icon** (`start_qtapp.sh`): starts qtapp if not running, or brings it to front via SIGUSR1.
  Set `QTAPP_URL` and optionally `QTAPP_INTERVAL` as env vars.
- **Cloud icon** (`start_qingping.sh`): sends SIGUSR2 to qtapp, hiding it so QingSnow2App is visible.

### Weston config

Copy the custom config to the device:

```bash
adb push scripts/weston.ini /etc/xdg/weston/weston.ini
```

Then restart weston to pick up the new launcher icons.

## App switching

Both QingSnow2App and qtapp run as Wayland clients under Weston. Switching between them:

- **Show qtapp:** tap the window icon, or `kill -USR1 $(pgrep -x qtapp)`
- **Show QingSnow2App:** tap the cloud icon, or `kill -USR2 $(pgrep -x qtapp)`

The SIGUSR1 handler hides then re-shows the window, which forces Weston to give it focus.
The SIGUSR2 handler hides the window, revealing QingSnow2App underneath.
