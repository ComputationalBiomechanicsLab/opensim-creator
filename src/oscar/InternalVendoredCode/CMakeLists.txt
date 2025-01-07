# glob all vendored code into a single static library
file(GLOB_RECURSE OSC_VENDORED_CODE_SOURCES
	LIST_DIRECTORIES FALSE
	src/*.cpp
	src/*.c
	src/*.h
)
add_library(oscar_internal_code STATIC ${OSC_VENDORED_CODE_SOURCES} src/lunasvg/lunasvg.cpp)

target_include_directories(oscar_internal_code PRIVATE include/ include/plutovg)
set_target_properties(oscar_internal_code PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(oscar_internal_code PRIVATE cxx_std_17)
target_compile_definitions(oscar_internal_code PRIVATE
	LUNASVG_BUILD_STATIC

	# this is necessary until we move away from our legacy `imgui.ini` and
	# `imgui_base_config.ini` files because existing users have already encoded
	# the legacy CRC checks into their local `imgui.ini` file (and we don't want
	# it to screw up when they update the software)
	IMGUI_USE_LEGACY_CRC32_ADLER
)
