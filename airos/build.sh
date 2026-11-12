#!/usr/bin/env bash
# airos/build.sh ARCH [ninja targets...]
#
# Cross-builds Telegram for air/OS from Linux:
#   1. airos/setup-sysroot.sh: the air/OS SDK sysroot + HaikuPorts packages
#   2. host tools: QtSvg for the host Qt, then codegen_{emoji,lang,style}
#   3. the cross build itself (CMake + Ninja) in $TDESKTOP_BUILD
#
# Environment:
#   AIROS_CI            airos-ci checkout (SDK paths, HaikuPorts resolver)
#   TDESKTOP_WORK       work area (default /data3/airos/tdesktop-work)
#   TDESKTOP_HOST_QT    host Qt of the target Qt version (moc, rcc, qsb)
#   TDESKTOP_API_ID / TDESKTOP_API_HASH
#                       Telegram API credentials; default: the ones Debian's
#                       telegram-desktop package builds with (as HaikuPorts)
#   JOBS                parallel jobs
set -euo pipefail
ARCH=${1:?usage: build.sh ARCH [targets...]}
shift
ROOT=$(cd "$(dirname "$0")/.." && pwd)
AIROS_CI=${AIROS_CI:-/mnt/HaikuWork/src/airos-ci}
WORK=${TDESKTOP_WORK:-/data3/airos/tdesktop-work}
JOBS=${JOBS:-$(nproc)}
HOSTQT=${TDESKTOP_HOST_QT:-/data1/airos/toolchain/proton-bridge/qt-6.10.3}
QT_VERSION=6.10.3
SYSROOT_DIR=$WORK/sysroot-$ARCH
HOST=$WORK/host
BUILD=${TDESKTOP_BUILD:-$WORK/build-$ARCH}
note() { printf '\n== %s\n' "$*"; }

note "sysroot"
AIROS_CI=$AIROS_CI "$ROOT/airos/setup-sysroot.sh" "$ARCH" "$SYSROOT_DIR"
. "$AIROS_CI/lib/env.sh"
export AIROS_ARCH=$ARCH
. "$AIROS_CI/lib/sdk.sh"

note "host tools"
HOSTSVG=$HOST/qtsvg-$QT_VERSION
if [[ ! -f $HOSTSVG/.done ]]; then
	tarball=$(ls "$AIROS_CACHE"/proton-bridge/qtsvg-$QT_VERSION.tar.xz 2>/dev/null | head -n 1 || true)
	if [[ -z $tarball ]]; then
		tarball=$HOST/qtsvg-$QT_VERSION.tar.xz
		mkdir -p "$HOST"
		curl -fsSL -o "$tarball" "https://download.qt.io/official_releases/qt/${QT_VERSION%.*}/$QT_VERSION/submodules/qtsvg-everywhere-src-$QT_VERSION.tar.xz"
	fi
	rm -rf "$HOST/qtsvg-src" "$HOST/qtsvg-build"
	mkdir -p "$HOST/qtsvg-src"
	tar -xf "$tarball" -C "$HOST/qtsvg-src" --strip-components=1
	"$HOSTQT/bin/qt-cmake" -S "$HOST/qtsvg-src" -B "$HOST/qtsvg-build" -G Ninja \
		-DCMAKE_INSTALL_PREFIX="$HOSTSVG" -DQT_BUILD_EXAMPLES=OFF -DQT_BUILD_TESTS=OFF \
		> "$HOST/qtsvg.log"
	cmake --build "$HOST/qtsvg-build" -j"$JOBS" >> "$HOST/qtsvg.log"
	cmake --install "$HOST/qtsvg-build" >> "$HOST/qtsvg.log"
	rm -rf "$HOST/qtsvg-src" "$HOST/qtsvg-build"
	touch "$HOSTSVG/.done"
fi
CODEGEN=$HOST/codegen
cmake -S "$ROOT/airos/host-codegen" -B "$HOST/codegen-build" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOSTQT;$HOSTSVG" \
	-DCMAKE_INSTALL_PREFIX="$CODEGEN" > /dev/null
cmake --build "$HOST/codegen-build" -j"$JOBS"
cmake --install "$HOST/codegen-build" > /dev/null

note "Telegram ($ARCH)"
API_ID=${TDESKTOP_API_ID:-611335}
API_HASH=${TDESKTOP_API_HASH:-d524b414d21f4d37f08684c1df41ac9c}
export LD_LIBRARY_PATH=$HOSTQT/lib:$HOSTSVG/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
if [[ ! -f $BUILD/build.ninja ]]; then
	cmake -S "$ROOT" -B "$BUILD" -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE="$ROOT/airos/toolchain.cmake" \
		-DAIROS_ARCH="$ARCH" -DAIROS_SYSROOT="$SYSROOT_DIR" -DAIROS_CROSS="$CROSS" \
		-DQT_HOST_PATH="$HOSTQT" -DQT_HOST_PATH_CMAKE_DIR="$HOSTQT/lib/cmake" \
		-DTDESKTOP_HOST_CODEGEN="$CODEGEN/bin" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_FLAGS_RELEASE="-O2" -DCMAKE_C_FLAGS_RELEASE="-O2" \
		-DCMAKE_EXE_LINKER_FLAGS="-lnetwork -lbe -lmedia -specs=$UNWIND_SPECS" \
		-DCMAKE_SHARED_LINKER_FLAGS="-specs=$UNWIND_SPECS" \
		-DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
		-DBoost_USE_STATIC_LIBS=OFF \
		-DBUILD_SHARED_LIBS=ON \
		-DTDESKTOP_API_TEST=OFF \
		-DTDESKTOP_DISABLE_GTK_INTEGRATION=ON \
		-DDESKTOP_APP_DISABLE_AUTOUPDATE=ON \
		-DDESKTOP_APP_DISABLE_CRASH_REPORTS=ON \
		-DDESKTOP_APP_DISABLE_DBUS_INTEGRATION=ON \
		-DDESKTOP_APP_DISABLE_GTK_INTEGRATION=ON \
		-DDESKTOP_APP_DISABLE_JEMALLOC=ON \
		-DDESKTOP_APP_ENABLE_LTO=OFF \
		-DDESKTOP_APP_DISABLE_WEBKITGTK=ON \
		-DDESKTOP_APP_DISABLE_SCUDO=ON \
		-DDESKTOP_APP_DISABLE_WAYLAND_INTEGRATION=ON \
		-DDESKTOP_APP_DISABLE_X11_INTEGRATION=ON \
		-DDESKTOP_APP_USE_PACKAGED_FONTS=ON \
		-DDESKTOP_APP_USE_HUNSPELL_ONLY=ON \
		-DTDESKTOP_USE_PACKAGED_TGVOIP=OFF \
		-DLIBTGVOIP_DISABLE_ALSA=ON \
		-DLIBTGVOIP_DISABLE_PULSEAUDIO=ON \
		-DTDESKTOP_API_ID="$API_ID" \
		-DTDESKTOP_API_HASH="$API_HASH" \
		-DLZ4_INCLUDE_DIRS="$SYSROOT_DIR/boot/system/develop/headers" \
		-Wno-dev
fi
ninja -C "$BUILD" -j"$JOBS" "${@:-Telegram}"
