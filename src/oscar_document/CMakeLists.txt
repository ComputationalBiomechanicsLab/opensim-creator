add_library(oscar_document STATIC
    PropertyDescription.hpp
    PropertyTableEntry.hpp
    NodePath.cpp
    NodePath.hpp
    StringName.cpp
    StringName.hpp
    Variant.cpp
    Variant.hpp
    VariantType.cpp
    VariantType.hpp
)

target_include_directories(oscar_document PUBLIC

    # so that `#include "oscar_document/HEADER.hpp"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_document PUBLIC
    oscar_compiler_configuration
    oscar
)
