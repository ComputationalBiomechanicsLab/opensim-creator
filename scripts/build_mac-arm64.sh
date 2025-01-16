#!/usr/bin/env bash

OSC_CMAKE_CONFIG_EXTRA="-Werror=dev -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX14.2.sdk -DCMAKE_OSX_DEPLOYMENT_TARGET=14 -DCMAKE_OSX_ARCHITECTURES=arm64" OSC_BUILD_CONCURRENCY=10 ./scripts/build_mac.sh

