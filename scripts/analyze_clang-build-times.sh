#!/usr/bin/env bash

rm -rf build-clangbuildanalyzer  # clean out existing build
CC=clang CXX=clang++ CXXFLAGS=-ftime-trace cmake -G Ninja -S . -B build-clangbuildanalyzer -DCMAKE_PREFIX_PATH=${PWD}/third_party-install-Debug -DOSC_BUILD_OPENSIMCREATOR=OFF
ClangBuildAnalyzer --start build-clangbuildanalyzer/
cmake --build build-clangbuildanalyzer/
ClangBuildAnalyzer --stop build-clangbuildanalyzer/ cba-capture
