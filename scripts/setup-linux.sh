#!/usr/bin/env bash
# Linux build bootstrap for LiViM.
#
# vcpkg is the default and is mandatory for release/CI builds. Use --system only for local builds
# that use distro Qt6/OpenCV/FFmpeg. Dependency selection is represented by the CMake preset; CMake
# performs the actual find_package validation.
# Idempotent -- safe to re-run.
#
# Usage:
#   scripts/setup-linux.sh [--configure] [--compiler gcc|clang] [--no-install] [--system|--vcpkg]
#     --configure         run cmake configure at the end
#     --compiler gcc|clang compiler preset family to use (default: gcc)
#     --no-install        skip distro package installation
#     --system            use distro Qt6/OpenCV/FFmpeg (opt-in)
#     --vcpkg             use vcpkg (default)
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DO_INSTALL=1
DO_CONFIGURE=0
COMPILER=gcc
FORCE_SYSTEM=0
FORCE_VCPKG=0

while [ $# -gt 0 ]; do
    case "$1" in
        --configure)   DO_CONFIGURE=1 ;;
        --no-install)  DO_INSTALL=0 ;;
        --compiler)    COMPILER="${2:?--compiler needs gcc|clang}"; shift ;;
        --system)      FORCE_SYSTEM=1 ;;
        --vcpkg)       FORCE_VCPKG=1 ;;
        -h|--help)
            sed -n '2,15p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *)             echo "unknown arg: $1" >&2; exit 2 ;;
    esac
    shift
done

[ "$COMPILER" = gcc ] || [ "$COMPILER" = clang ] || { echo "--compiler must be gcc or clang" >&2; exit 2; }
[ "$FORCE_SYSTEM" = 1 ] && [ "$FORCE_VCPKG" = 1 ] && { echo "--system and --vcpkg are mutually exclusive" >&2; exit 2; }

# vcpkg is the default; --system selects the distro Qt6/OpenCV/FFmpeg.
USE_SYSTEM=$FORCE_SYSTEM

APT_PKGS=(
    build-essential ninja-build cmake pkg-config nasm git curl zip unzip tar
    autoconf autoconf-archive automake libtool patchelf
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
    autoconf autoconf-archive automake libtool patchelf
    mesa-libGL-devel mesa-libGLU-devel mesa-libEGL-devel
    libX11-devel libXext-devel libXfixes-devel libXi-devel libXrender-devel
    libxcb-devel xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel xcb-util-wm-devel
    xcb-util-renderutil-devel xcb-util-cursor-devel
    libxkbcommon-devel libxkbcommon-x11-devel fontconfig-devel freetype-devel
    wayland-devel wayland-protocols-devel
)
PACMAN_PKGS=(
    base-devel clang cmake ninja nasm git curl zip unzip tar
    autoconf autoconf-archive automake libtool patchelf
    mesa glu libglvnd
    libx11 libxext libxfixes libxi libxrender
    libxcb xcb-util xcb-util-image xcb-util-keysyms xcb-util-wm xcb-util-renderutil xcb-util-cursor
    libxkbcommon libxkbcommon-x11 fontconfig freetype2
    wayland wayland-protocols
)

# Qt6/OpenCV/FFmpeg from the distro, installed only for --system; skipped for the default vcpkg
# build, where vcpkg builds them from source.
APT_SYSTEM_PKGS=(qt6-base-dev qt6-wayland-dev libopencv-dev ffmpeg)
DNF_SYSTEM_PKGS=(qt6-qtbase-devel qt6-qtwayland-devel opencv-devel ffmpeg-devel)
PACMAN_SYSTEM_PKGS=(qt6-base qt6-wayland opencv ffmpeg)

detect_pm() {
    if command -v apt-get >/dev/null 2>&1; then echo apt
    elif command -v dnf >/dev/null 2>&1; then echo dnf
    elif command -v pacman >/dev/null 2>&1; then echo pacman
    else echo unknown; fi
}

install_system_deps() {
    local pm; pm="$(detect_pm)"
    local sudo_cmd=(); [ "$(id -u)" -eq 0 ] || sudo_cmd=(sudo)
    echo "== installing Linux build dependencies via: $pm =="
    case "$pm" in
        apt)
            "${sudo_cmd[@]}" apt-get update
            "${sudo_cmd[@]}" apt-get install -y "${APT_PKGS[@]}"
            if [ "$USE_SYSTEM" = 1 ]; then
                "${sudo_cmd[@]}" apt-get install -y "${APT_SYSTEM_PKGS[@]}"
            fi
            ;;
        dnf)
            "${sudo_cmd[@]}" dnf install -y "${DNF_PKGS[@]}"
            if [ "$USE_SYSTEM" = 1 ]; then
                "${sudo_cmd[@]}" dnf install -y "${DNF_SYSTEM_PKGS[@]}"
            fi
            ;;
        pacman)
            "${sudo_cmd[@]}" pacman -S --needed --noconfirm "${PACMAN_PKGS[@]}"
            if [ "$USE_SYSTEM" = 1 ]; then
                # Ask pacman whether the dependency is already satisfied, rather than checking
                # for one concrete package name. This accepts opencv-cuda and any other package
                # that provides opencv, while avoiding the conflict with stock opencv.
                local sys_pkgs=()
                for pkg in "${PACMAN_SYSTEM_PKGS[@]}"; do
                    if [ "$pkg" = "opencv" ] && pacman -T opencv >/dev/null 2>&1; then
                        echo "== detected an installed package that provides opencv; skipping stock opencv =="
                        continue
                    fi
                    sys_pkgs+=("$pkg")
                done
                "${sudo_cmd[@]}" pacman -S --needed --noconfirm "${sys_pkgs[@]}"
            fi
            ;;
        *)
            echo "!! Unsupported package manager; install the documented Linux dependencies manually." >&2
            exit 1
            ;;
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
    echo "== VCPKG_ROOT is set for this script run: $target =="
}

[ "$DO_INSTALL" -eq 1 ] && install_system_deps

# vcpkg is the default. The release workflow always uses the vcpkg presets, so runner-installed
# packages can never change the release dependency source. --system picks the distro Qt6/OpenCV/FFmpeg.
if [ "$USE_SYSTEM" = 1 ]; then
    PRESET="${COMPILER}-system"
else
    ensure_vcpkg
    PRESET="$COMPILER"
fi

if [ "$DO_CONFIGURE" -eq 1 ]; then
    cmake --preset "$PRESET" -S "$REPO_ROOT"
    echo "== done. build with: cmake --build --preset ${PRESET}-release =="
else
    if [ "$USE_SYSTEM" = 1 ]; then
        echo "== setup complete (system). Next:"
    else
        echo "== setup complete (vcpkg). Next:"
    fi
    echo "     cmake --preset $PRESET && cmake --build --preset ${PRESET}-release"
fi
