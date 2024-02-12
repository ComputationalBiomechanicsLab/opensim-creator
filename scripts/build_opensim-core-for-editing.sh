#!/usr/bin/env bash

cmake -S dependencies -B deps-build -DOPENSIM_WITH_CASADI=OFF -DOPENSIM_WITH_TROPTER=OFF -DCMAKE_INSTALL_PREFIX=${PWD}/deps-install
cmake --build deps-build -j$(nproc) --config RelWithDebInfo

# open opensim-core dir in Visual Studio
# args to customize:
# -DOPENSIM_DEPENDENCIES_DIR:PATH=${CMAKE_CURRENT_BINARY_DIR}/opensim-core-dependencies-install
# -DOPENSIM_WITH_CASADI:BOOL=OFF
# -DOPENSIM_WITH_TROPTER:BOOL=OFF
