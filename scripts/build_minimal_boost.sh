#!/bin/bash
# build_minimal_boost.sh - Build minimal Boost libraries for hbthreads
#
# This script builds a minimal subset of Boost libraries required by the hbthreads
# project. It creates a static, single-threaded, release build optimized for
# high-performance applications.
#
# Required Boost components for hbthreads:
# - context: Coroutine context switching (LightThread implementation)
# - container: Memory pools and flat containers (Pointer memory management)
# - smart_ptr: Intrusive pointers (Pointer class implementation)
# - fiber: Stack allocation (LightThread stack management)
#
# Optional components (may be removed if not needed):
# - regex: Currently included but not used in hbthreads
# - program_options: Currently included but not used in hbthreads
#
# Usage:
#   ./build_minimal_boost.sh
#
# Environment variables:
#   INSTALL_DIR: Installation prefix (default: /usr/local)
#   BUILD_DIR: Build directory (default: /tmp)
#   TAG: Boost version tag (default: master)
#
# Examples:
#   TAG=boost-1.78.0 INSTALL_DIR=/opt/boost ./build_minimal_boost.sh
#   BUILD_DIR=/tmp/boost_build TAG=boost-1.81.0 ./build_minimal_boost.sh

# Exit on any error, show commands, fail on pipe errors
set -exo pipefail

# Configuration with environment variable overrides
INSTALL_DIR=${INSTALL_DIR:-/usr/local}
BUILD_DIR=${BUILD_DIR:-/tmp}
TAG=${TAG:-master}

echo "Building minimal Boost with the following configuration:"
echo "  INSTALL_DIR: $INSTALL_DIR"
echo "  BUILD_DIR: $BUILD_DIR"
echo "  TAG: $TAG"
echo ""

# Ensure build directory exists
cd "$BUILD_DIR"

# Clone boost repository if not present
if [ ! -d boost ]; then
    echo "Cloning Boost repository..."
    git clone https://github.com/boostorg/boost.git
fi

# Enter boost directory and checkout specified tag
cd boost
echo "Checking out Boost $TAG..."
git checkout "$TAG"

# Update required submodules for minimal build
# Note: This includes more submodules than strictly necessary to ensure
# all dependencies are available, but only required libraries are built
echo "Updating Boost submodules..."
submodules="
    tools/build
    tools/boost_install
    libs/config
    libs/core
    libs/move
    libs/headers
    libs/container
    libs/smart_ptr
    libs/intrusive
    libs/throw_exception
    libs/exception
    libs/assert
    libs/fiber
    libs/context
"

for submodule in $submodules; do
    echo "  Updating $submodule..."
    git submodule update --init "$submodule"
done

# Bootstrap the build system
echo "Bootstrapping Boost build system..."
./bootstrap.sh --prefix="$INSTALL_DIR"

# Install Boost headers
echo "Installing Boost headers..."
./b2 headers

# Build and install minimal Boost libraries
# - Static linking for better performance and simpler deployment
# - Single-threaded to avoid locking overhead in HFT applications
# - Release build with optimizations
# - Only build required libraries to minimize build time and size
echo "Building and installing Boost libraries..."
./b2 install -q -a \
    --prefix="$INSTALL_DIR" \
    --build-type=minimal \
    --layout=system \
    --disable-icu \
    --with-container \
    --with-context \
    --with-fiber \
    variant=release \
    link=static \
    runtime-link=static \
    threading=single \
    address-model=64 \
    architecture=x86

echo ""
echo "Boost build completed successfully!"
echo "Libraries installed to: $INSTALL_DIR"
echo ""
echo "Built libraries:"
echo "  libboost_container.a"
echo "  libboost_context.a"
echo "  libboost_fiber.a"
echo ""
echo "To use this Boost installation with hbthreads:"
echo "  cmake -DUSE_LOCAL_BOOST=ON -DLOCAL_BOOST_PREFIX=$INSTALL_DIR .."
