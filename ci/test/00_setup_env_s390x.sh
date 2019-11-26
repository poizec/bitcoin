#!/usr/bin/env bash
#
# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export HOST=s390x-unknown-linux-gnu
export DOCKER_NAME_TAG=s390x/ubuntu:18.04
export PACKAGES="clang llvm libssl1.0-dev libevent-dev bsdmainutils libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-test-dev libboost-thread-dev libdb5.3++-dev"
export NO_DEPENDS=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export GOAL="install"
export SYSCOIN_CONFIG="--enable-reduce-exports --with-incompatible-bdb --with-gui=no"

lscpu
