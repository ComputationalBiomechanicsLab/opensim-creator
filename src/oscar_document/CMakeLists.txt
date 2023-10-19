add_library(oscar_document STATIC
    NodePath.cpp
    NodePath.hpp
)

target_include_directories(oscar_document PUBLIC

    # so that `#include "oscar_document/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_document PUBLIC
    oscar_compiler_configuration
    oscar
)
