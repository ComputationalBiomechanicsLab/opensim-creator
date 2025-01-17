find_package(Python 3.8 COMPONENTS Development.Module REQUIRED)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/nanobind)
nanobind_add_module(_my_ext_impl src/my_ext.cpp)
target_link_libraries(_my_ext_impl PRIVATE OpenSimCreator)
set_target_properties(_my_ext_impl PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/my_ext
)
