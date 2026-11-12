#!/usr/bin/env bash
# airos/setup-sysroot.sh ARCH [SYSROOT]
#
# A cross sysroot for Telegram on air/OS: the air/OS SDK's sysroot plus the
# HaikuPorts packages Telegram builds against (airos/haikuports-deps.txt),
# extracted into its boot/system. Links and pkg-config files that name
# /boot/system or /packages/NAME/.self are made to lead into the sysroot.
#
# Needs an airos-ci checkout (AIROS_CI, default /mnt/HaikuWork/src/airos-ci)
# for its SDK and its HaikuPorts resolver. The sysroot is rebuilt only when
# the SDK revision or the package list changes.
set -euo pipefail
ARCH=${1:?usage: setup-sysroot.sh ARCH [SYSROOT]}
HERE=$(cd "$(dirname "$0")" && pwd)
AIROS_CI=${AIROS_CI:-/mnt/HaikuWork/src/airos-ci}
. "$AIROS_CI/lib/env.sh"
export AIROS_ARCH=$ARCH
. "$AIROS_CI/lib/sdk.sh"
OUT=${2:-${TDESKTOP_SYSROOT:-/data3/airos/tdesktop-work/sysroot-$ARCH}}
system=$OUT/boot/system

mapfile -t reqs < <(grep -v -E '^\s*(#|$)' "$HERE/haikuports-deps.txt")
key="$(cat "$AIROS_SDK/$AIROS_SDK_NAME/revision" 2>/dev/null || echo "$SYSROOT") ${reqs[*]}"
if [[ -f $OUT/.key && $(cat "$OUT/.key") == "$key" ]]; then
	echo "sysroot $OUT is current"
	exit 0
fi
[[ ! -d $OUT ]] || chmod -R u+w "$OUT"
rm -rf "$OUT"
mkdir -p "$OUT"
rsync -a --exclude=/boot/system/data/ssl/private "$SYSROOT/" "$OUT/"
AIROS_CACHE=$AIROS_CACHE AIROS_SDK=$AIROS_SDK \
	python3 "$AIROS_CI/deps/haikuports.py" stage "$ARCH" "$system" "${reqs[@]}" \
	> "$OUT/haikuports.txt"
chmod -R u+w "$OUT"
python3 "$AIROS_CI/lib/fix-devel-links.py" "$system"
# pkg-config files carry the package's own directory as their prefix
find "$system" -name '*.pc' -print0 | xargs -0 sed -i -E \
	-e 's#/packages/[^/ ]+/\.self#/boot/system#g'
# libtool archives point libtool at /boot/system; nothing here uses them
find "$system" -name '*.la' -delete
echo "$key" > "$OUT/.key"
echo "sysroot $OUT ready ($(wc -l < "$OUT/haikuports.txt") HaikuPorts packages)"
