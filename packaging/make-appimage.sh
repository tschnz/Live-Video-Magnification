#!/usr/bin/env bash
# Build a portable AppImage of `livim` via linuxdeploy + linuxdeploy-plugin-qt.
# glibc floor: the AppImage bundles everything except glibc and GPU/GL drivers, so build on the
# oldest distro you want to support (e.g. an Ubuntu 22.04 container).
#
# Usage: packaging/make-appimage.sh [<build-preset-dir> [<config>]]   (defaults: build/gcc Release)
# Overrides: BUILD_DIR, CONFIG, QMAKE, OUTPUT.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PKG_DIR="$REPO_ROOT/packaging"
ASSETS_DIR="$PKG_DIR/assets"
TOOLS_DIR="$PKG_DIR/.tools"
DIST_DIR="$REPO_ROOT/dist"

BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/${1:-build/gcc}}"
CONFIG="${CONFIG:-${2:-Release}}"
VCPKG_TREE="$REPO_ROOT/build/vcpkg_installed"
# Target tree we bundle from (dynamic-linkage Qt + .so plugins).
VCPKG_INSTALLED="${VCPKG_INSTALLED:-$VCPKG_TREE/x64-linux-dynamic}"

BIN="$BUILD_DIR/$CONFIG/livim"
[ -x "$BIN" ] || { echo "error: livim binary not found at $BIN (build it first, e.g. cmake --build --preset gcc-app)"; exit 1; }

# linuxdeploy-plugin-qt needs a qmake whose `-query` reports the bundled tree's paths; run the
# vcpkg qmake6 through a shim qt.conf pinned to the dynamic target tree.
if [ -z "${QMAKE:-}" ]; then
    HOST_QMAKE="$(find "$VCPKG_TREE" -path '*/tools/Qt6/bin/qmake6' -type f 2>/dev/null | head -1)"
    [ -x "$HOST_QMAKE" ] || { echo "error: no host qmake6 found under $VCPKG_TREE (configure first); or set QMAKE=..."; exit 1; }
    HOST_TREE="$(cd "$(dirname "$HOST_QMAKE")/../../.." && pwd)"   # <tree> from <tree>/tools/Qt6/bin
    SHIM="$BUILD_DIR/.qmake-shim"
    mkdir -p "$SHIM"
    cp -f "$HOST_QMAKE" "$SHIM/qmake"
    # Copying qmake breaks its $ORIGIN-relative RPATH; re-point the copy at the target tree.
    command -v patchelf >/dev/null || { echo "error: patchelf not found (required to relink the qmake shim)"; exit 1; }
    patchelf --set-rpath "$VCPKG_INSTALLED/lib" "$SHIM/qmake"
    cat > "$SHIM/qt.conf" <<EOF
[Paths]
Prefix=$VCPKG_INSTALLED
Headers=include/Qt6/
Libraries=lib
Plugins=Qt6/plugins
Qml2Imports=Qt6/qml
ArchData=share/Qt6
Data=share/Qt6
Binaries=bin
LibraryExecutables=tools/Qt6/bin
HostPrefix=$HOST_TREE
HostData=share/Qt6
HostBinaries=tools/Qt6/bin
EOF
    QMAKE="$SHIM/qmake"
fi
[ -x "$QMAKE" ] || { echo "error: qmake not found at $QMAKE (set QMAKE=...)"; exit 1; }

mkdir -p "$TOOLS_DIR"
fetch_tool() { # <name> <url> -> tool path on stdout
    local out="$TOOLS_DIR/$1"
    if [ ! -x "$out" ]; then
        echo "-- fetching $1" >&2
        curl -fSL "$2" -o "$out"
        chmod +x "$out"
    fi
    echo "$out"
}
ARCH="$(uname -m)"
LINUXDEPLOY="$(fetch_tool linuxdeploy-$ARCH.AppImage \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage")"
fetch_tool linuxdeploy-plugin-qt-$ARCH.AppImage \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage" >/dev/null
export PATH="$TOOLS_DIR:$PATH"

APPDIR="$BUILD_DIR/AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp "$BIN" "$APPDIR/usr/bin/livim"

# Bundle the Wayland plugin set in addition to the auto-detected xcb platform plugin.
export QMAKE
export EXTRA_PLATFORM_PLUGINS="libqwayland.so"
export EXTRA_QT_PLUGINS="wayland-decoration-client;wayland-graphics-integration-client;wayland-shell-integration"

mkdir -p "$DIST_DIR"

# Output name matches CPack's scheme: <Name>-<Version>-Linux-<arch>.AppImage (version from
# CMakeLists' project(... VERSION ...); override with VERSION=...).
VERSION="${VERSION:-$(sed -n 's/^[[:space:]]*project([[:space:]]*LiViM[[:space:]]\+VERSION[[:space:]]\+\([0-9.]\+\).*/\1/p' "$REPO_ROOT/CMakeLists.txt" | head -1)}"
[ -n "$VERSION" ] || { echo "error: could not read version from CMakeLists.txt (set VERSION=...)"; exit 1; }
case "$ARCH" in
    aarch64) NAME_ARCH="arm64" ;;
    *)       NAME_ARCH="$ARCH" ;;
esac
OUT_FILE="${OUTPUT:-$DIST_DIR/LiViM-$VERSION-Linux-$NAME_ARCH.AppImage}"
# Newer linuxdeploy-plugin-appimage reads $LDAI_OUTPUT, older $OUTPUT — set both.
export OUTPUT="$OUT_FILE"
export LDAI_OUTPUT="$OUT_FILE"

"$LINUXDEPLOY" --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/livim" \
    --desktop-file "$ASSETS_DIR/livim.desktop" \
    --icon-file "$ASSETS_DIR/livim.png" \
    --plugin qt \
    --output appimage

echo "done -> $OUT_FILE"
