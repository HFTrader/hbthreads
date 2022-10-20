#!/bin/bash
# Reasons to use this script:
# 1. you do not want to install the entire boost from your distro
# 2. you need a static boost to use in your project 
# 3. you want a single 
# 3. you need a newer version of boost, maybe the latest 
# This script will build a minimal boost with just what is needed 
# for this project. Unlike the full boost, this should take only 
# a few seconds to compile
set -exo pipefail

# defaults to instaling on /usr/local and building on /tmp
# you can override this by prefixing the call as in 
# $ TAG=boost-1.78.0 INSTALL_DIR=/ssd/install BUILD_DIR=/ssd/tmp ./build_minimal_boost.sh 
INSTALL_DIR=${INSTALL_DIR:-/usr/local}
BUILD_DIR=${BUILD_DIR:-/tmp}
TAG=${TAG:-master}

# clone if not yet
cd $BUILD_DIR
if [ ! -d boost ]; then
    git clone https://github.com/boostorg/boost.git
fi

# checkout our tag
cd boost
git checkout $TAG 

# checkout/update submodules
allsm=" tools/build                                                                              
        tools/boost_install
        libs/regex
        libs/config
        libs/core
        libs/move
        libs/program_options
        libs/headers
        libs/container
        libs/smart_ptr
        libs/intrusive
        libs/throw_exception
        libs/exception
        libs/assert
	libs/fiber
        libs/context"
for sm in $allsm; do
    git submodule update --init $sm
done

# create the b2 executable
./bootstrap.sh --prefix=$INSTALL_DIR

# install headers 
./b2 headers

# build it static/single/release
./b2 install -q -a \
        --prefix=$INSTALL_DIR \
        --build-type=minimal \
        --layout=system \
        --disable-icu \
        --with-regex \
        --with-context \
        --with-program_options \
        --with-container \
        variant=release link=static runtime-link=static \
        threading=single address-model=64 architecture=x86 
