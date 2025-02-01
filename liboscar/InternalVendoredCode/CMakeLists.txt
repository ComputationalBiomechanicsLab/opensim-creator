# glob all vendored code into a single static library
file(GLOB_RECURSE OSC_VENDORED_CODE_SOURCES
	LIST_DIRECTORIES FALSE
	src/*.cpp
	src/*.c
	src/*.h
)
add_library(oscar_internal_code STATIC ${OSC_VENDORED_CODE_SOURCES})
target_link_libraries(oscar_internal_code PUBLIC imgui)
target_include_directories(oscar_internal_code PUBLIC include/)
set_target_properties(oscar_internal_code PROPERTIES
	CXX_EXTENSIONS OFF

	# ensure including a vendored code header file doesn't
	# trigger higher warning levels
	SYSTEM ON
)
# ensure linter doesn't run on vendoered code
set_property(TARGET oscar_internal_code PROPERTY CXX_CLANG_TIDY)
set_property(TARGET oscar_internal_code PROPERTY C_CLANG_TIDY)
target_compile_features(oscar_internal_code PRIVATE cxx_std_17)
