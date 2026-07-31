#!/usr/bin/env bash
# Linux build bootstrap for LiViM: installs system build deps (apt/dnf/pacman) and sets up vcpkg.
# Idempotent — safe to re-run.
#
# Usage:
#   scripts/setup-linux.sh [--configure] [--compiler gcc|clang] [--no-install]
#     --configure         run `cmake --preset <compiler>` at the end
#     --compiler gcc|clang compiler preset to configure with (default: gcc)
#     --no-install        skip system-package install
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DO_INSTALL=1
DO_CONFIGURE=0
COMPILER=gcc

while [ $# -gt 0 ]; do
    case "$1" in
        --configure)   DO_CONFIGURE=1 ;;
        --no-install)  DO_INSTALL=0 ;;
        --compiler)    COMPILER="${2:?--compiler needs gcc|clang}"; shift ;;
        -h|--help)     sed -n '2,9p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)             echo "unknown arg: $1" >&2; exit 2 ;;
    esac
    shift
done
[ "$COMPILER" = gcc ] || [ "$COMPILER" = clang ] || { echo "--compiler must be gcc or clang" >&2; exit 2; }

APT_PKGS=(
    build-essential ninja-build cmake pkg-config nasm git curl zip unzip tar
    autoconf autoconf-archive automake libtool
    libgl1-mesa-dev libglu1-mesa-dev libegl1-mesa-dev
    libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev
    libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev
    libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev
    libxcb-render-util0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxcb-xinput-dev
    libxkbcommon-dev libxkbcommon-x11-dev libfontconfig1-dev libfreetype-dev
    libwayland-dev wayland-protocols
)
DNF_PKGS=(
    gcc-c++ clang ninja-build cmake pkgconf-pkg-config nasm make git curl zip unzip tar
    autoconf autoconf-archive automake libtool
    mesa-libGL-devel mesa-libGLU-devel mesa-libEGL-devel
    libX11-devel libXext-devel libXfixes-devel libXi-devel libXrender-devel
    libxcb-devel xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel xcb-util-wm-devel
    xcb-util-renderutil-devel xcb-util-cursor-devel
    libxkbcommon-devel libxkbcommon-x11-devel fontconfig-devel freetype-devel
    wayland-devel wayland-protocols-devel
)
PACMAN_PKGS=(
    base-devel clang cmake ninja nasm git curl zip unzip tar
    autoconf autoconf-archive automake libtool
    mesa glu libglvnd
    libx11 libxext libxfixes libxi libxrender
    libxcb xcb-util xcb-util-image xcb-util-keysyms xcb-util-wm xcb-util-renderutil xcb-util-cursor
    libxkbcommon libxkbcommon-x11 fontconfig freetype2
    wayland wayland-protocols
)

detect_pm() {
    if command -v apt-get >/dev/null 2>&1; then echo apt
    elif command -v dnf  >/dev/null 2>&1; then echo dnf
    elif command -v pacman >/dev/null 2>&1; then echo pacman
    else echo unknown; fi
}

install_system_deps() {
    local pm; pm="$(detect_pm)"
    local SUDO=""; [ "$(id -u)" -eq 0 ] || SUDO="sudo"
    echo "== installing system dependencies via: $pm =="
    case "$pm" in
        apt)    $SUDO apt-get update && $SUDO apt-get install -y "${APT_PKGS[@]}" ;;
        dnf)    $SUDO dnf install -y "${DNF_PKGS[@]}" ;;
        pacman) $SUDO pacman -S --needed --noconfirm "${PACMAN_PKGS[@]}" ;;
        *)      echo "!! Unsupported package manager. Install the equivalents of the apt list in" \
                     "README.md manually, then re-run with --no-install." >&2; exit 1 ;;
    esac
}

ensure_vcpkg() {
    if [ -n "${VCPKG_ROOT:-}" ] && [ -x "$VCPKG_ROOT/vcpkg" ]; then
        echo "== using existing VCPKG_ROOT=$VCPKG_ROOT =="
        return
    fi
    local target="${VCPKG_ROOT:-$HOME/vcpkg}"
    if [ ! -d "$target/.git" ]; then
        echo "== cloning vcpkg into $target =="
        git clone https://github.com/microsoft/vcpkg "$target"
    fi
    if [ ! -x "$target/vcpkg" ]; then
        echo "== bootstrapping vcpkg =="
        "$target/bootstrap-vcpkg.sh" -disableMetrics
    fi
    export VCPKG_ROOT="$target"
    echo
    echo ">> VCPKG_ROOT is set for this script run. Add it to your shell so future builds find it:"
    echo "     bash/zsh:  export VCPKG_ROOT=\"$target\""
    echo "     fish:      set -Ux VCPKG_ROOT \"$target\""
    echo
}

[ "$DO_INSTALL" -eq 1 ] && install_system_deps
ensure_vcpkg

if [ "$DO_CONFIGURE" -eq 1 ]; then
    echo "== configuring (preset: $COMPILER) — first run compiles Qt+OpenCV+ffmpeg from source (~30-90 min) =="
    cmake --preset "$COMPILER" -S "$REPO_ROOT"
    echo "== done. build with:  cmake --build --preset ${COMPILER}-release =="
else
    echo "== setup complete. Next:"
    echo "     cmake --preset $COMPILER && cmake --build --preset ${COMPILER}-release"
fi
