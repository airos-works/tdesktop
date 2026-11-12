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

# The cross compiler: a copy of the air/OS SDK's, so that CI rebuilding
# its SDK (which holds the SDK lock for a long time) neither blocks nor
# changes a Telegram build under way. Refreshed when the SDK's changes.
sdk_env() { sed -n "s/^$1=//p" "$AIROS_SDK/$ARCH/env.sh"; }
TRIPLET=$(sdk_env TRIPLET)
SDK_CROSS=$(sdk_env CROSS)
TOOLS=$(sdk_env TOOLS)
CROSS_DIR=$WORK/cross-$ARCH
sdk_tools=$(dirname "$(dirname "$SDK_CROSS")")
if [[ ! -x $CROSS_DIR/bin/${TRIPLET}-g++ || $sdk_tools/bin/${TRIPLET}-gcc -nt $CROSS_DIR/bin/${TRIPLET}-gcc ]]; then
	note "cross toolchain snapshot"
	rsync -a --delete "$sdk_tools/" "$CROSS_DIR/"
	cp "$AIROS_SDK/$ARCH/shared-unwinder.specs" "$CROSS_DIR/"
fi
CROSS=$CROSS_DIR/bin/${TRIPLET}-
UNWIND_SPECS=$CROSS_DIR/shared-unwinder.specs

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
	-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$HOSTSVG" \
	-DCMAKE_INSTALL_PREFIX="$CODEGEN" > /dev/null
cmake --build "$HOST/codegen-build" -j"$JOBS"
cmake --install "$HOST/codegen-build" > /dev/null

note "Telegram ($ARCH)"
API_ID=${TDESKTOP_API_ID:-611335}
API_HASH=${TDESKTOP_API_HASH:-d524b414d21f4d37f08684c1df41ac9c}
sys=$SYSROOT_DIR/boot/system
export PKG_CONFIG_LIBDIR=$sys/develop/lib/pkgconfig:$sys/lib/pkgconfig:$sys/data/pkgconfig
export PKG_CONFIG_PATH= PKG_CONFIG_SYSROOT_DIR=$SYSROOT_DIR
export LD_LIBRARY_PATH=$HOSTQT/lib:$HOSTSVG/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
if [[ ! -f $BUILD/build.ninja || $ROOT/airos/build.sh -nt $BUILD/build.ninja ]]; then
	cmake -S "$ROOT" -B "$BUILD" -G Ninja \
		-DCMAKE_TOOLCHAIN_FILE="$ROOT/airos/toolchain.cmake" \
		-DAIROS_ARCH="$ARCH" -DAIROS_SYSROOT="$SYSROOT_DIR" -DAIROS_CROSS="$CROSS" \
		-DQT_HOST_PATH="$HOSTQT" -DQT_HOST_PATH_CMAKE_DIR="$HOSTQT/lib/cmake" \
		-DTDESKTOP_HOST_CODEGEN="$CODEGEN/bin" -DQSB_EXECUTABLE="$HOSTQT/bin/qsb" \
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
ninja -C "$BUILD" -j"$JOBS" ${NINJA_FLAGS:-} "${@:-Telegram}"

# Resources (signature, B_SINGLE_LAUNCH, version, icon) on the binary;
# TDESKTOP_SIGNATURE gives a test build a signature of its own, so it runs
# beside the installed Telegram instead of handing launches to it.
if [[ " ${*:-Telegram} " == *" Telegram "* ]]; then
	note "resources"
	signature=${TDESKTOP_SIGNATURE:-application/x-vnd.telegram}
	version=$(awk '$1 == "AppVersionStr" { print $2 }' "$ROOT/Telegram/build/version")
	IFS=. read -r major middle minor <<< "$version"
	sed -e "s|@APP_SIGNATURE@|$signature|" -e "s|@MAJOR@|$major|" \
		-e "s|@MIDDLE@|$middle|" -e "s|@MINOR@|${minor:-0}|" \
		-e "s|@LONG_INFO@|Telegram for air/OS|" \
		"$ROOT/airos/resources/telegram_desktop.rdef.in" > "$BUILD/Telegram.rdef"
	"$TOOLS/rc/rc" -o "$BUILD/Telegram.rsrc" "$BUILD/Telegram.rdef"
	"$TOOLS/xres" -o "$BUILD/bin/Telegram" "$BUILD/Telegram.rsrc"
fi
