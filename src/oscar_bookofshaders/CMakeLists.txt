add_library(oscar_bookofshaders STATIC
    BookOfShadersTab.cpp
    BookOfShadersTab.h
    BookOfShadersTabRegistry.cpp
    BookOfShadersTabRegistry.h
)

target_include_directories(oscar_bookofshaders PUBLIC

    # so that `#include "oscar_bookofshaders/HEADER.h"` works
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(oscar_bookofshaders PUBLIC
    oscar
)

set_target_properties(oscar_bookofshaders PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)
