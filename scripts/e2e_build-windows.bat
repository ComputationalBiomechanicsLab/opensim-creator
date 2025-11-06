python scripts/setup_venv-all.bat

call scripts/env_visual-studio.bat
call scripts/env_venv.bat

cmake -G Ninja -S third_party/ -B build/third_party/RelWithDebInfo -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=%cd%/build/third_party/RelWithDebInfo/install
cmake --build build/third_party/RelWithDebInfo
cmake -G Ninja -S . -B build/RelWithDebInfo -DCMAKE_PREFIX_PATH=%cd%/build/third_party/RelWithDebInfo/install/ -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/RelWithDebInfo
