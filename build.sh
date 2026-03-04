#!/bin/bash

# build.sh
#
# Purpose:
#   One-shot build script with vcpkg dependency resolution and CMake build.
#   All dependencies (fftw3, portaudio, libsndfile, imgui, glfw3, nativefiledialog)
#   are managed by vcpkg.
#
# Usage:
#   ./build.sh              # Release build, double precision (default)
#   ./build.sh debug        # Debug build,   double precision
#   ./build.sh float        # Release build, float precision  (fftwf)
#   ./build.sh float-debug  # Debug build,   float precision  (fftwf)
#   ./build.sh clean        # Remove build artifacts

set -e
cd "$(dirname "$0")"

# --- clean ---
if [ "${1}" = "clean" ]; then
    echo "Cleaning build artifacts..."
    rm -rf build
    exit 0
fi

# --- Preset selection ---
case "${1}" in
    debug)       PRESET="debug"       ;;
    float)       PRESET="float"       ;;
    float-debug) PRESET="float-debug" ;;
    *)           PRESET="default"     ;;
esac

# --- Check for CMake ---
if ! command -v cmake &> /dev/null; then
    echo "CMake not found. Installing..."
    if command -v brew &> /dev/null; then
        brew install cmake
    elif command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y cmake
    else
        echo "Error: cmake not found and no supported package manager available."
        exit 1
    fi
fi

# --- Set up vcpkg ---
if [ ! -d "vcpkg" ]; then
    echo "Cloning vcpkg..."
    git clone https://github.com/microsoft/vcpkg.git
fi

if [ ! -f "vcpkg/vcpkg" ] && [ ! -f "vcpkg/vcpkg.exe" ]; then
    echo "Bootstrapping vcpkg..."
    ./vcpkg/bootstrap-vcpkg.sh -disableMetrics
fi

# --- CMake configure & build ---
echo "Configuring with preset: ${PRESET}"
cmake --preset "${PRESET}"

echo "Building..."
cmake --build build

echo ""
echo "Build complete."
[ -f build/euterpe ]       && echo "  euterpe       -> build/euterpe"
[ -f build/euterpe_imgui ] && echo "  euterpe_imgui -> build/euterpe_imgui"
