REM this uses hard-coded paths etc. because this is mostly useful for rapidly getting
REM a build environment up on my Windows machine. Use at your own peril etc.

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cmake -S third_party -B osc-dependencies-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=%cd%/osc-dependencies-install -DOSCDEPS_OSIM_BUILD_OPENBLAS=ON
cmake --build osc-dependencies-build --config RelWithDebInfo
cmake -S . -B osc-build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=%cd%/osc-dependencies-install
cmake --build osc-build --config RelWithDebInfo
ctest --test-dir osc-build
