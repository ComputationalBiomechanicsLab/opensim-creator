
# add in all sundials source and headers here
file(GLOB src_files  ./src/*/*.c)
file(GLOB incl_files ./include/*/*.h)

foreach( src ${src_files} )
    list(APPEND SOURCE_GROUPS "Source Files\\CPodes")
    list(APPEND SOURCE_GROUP_FILES "${src}")
endforeach()

foreach( incl ${incl_files} )
    list(APPEND SOURCE_GROUPS "Header Files\\CPodes")
    list(APPEND SOURCE_GROUP_FILES "${incl}")
endforeach()

set(SOURCE_GROUPS ${SOURCE_GROUPS} PARENT_SCOPE)
set(SOURCE_GROUP_FILES ${SOURCE_GROUP_FILES} PARENT_SCOPE)

# append to the local scope copy, and then copy up to parent scope
list(APPEND SOURCE_FILES ${src_files})
set(SOURCE_FILES ${SOURCE_FILES} PARENT_SCOPE)

list(APPEND SOURCE_INCLUDE_FILES ${incl_files})
set(SOURCE_INCLUDE_FILES ${SOURCE_INCLUDE_FILES} PARENT_SCOPE)


# build tests that depend on source at this level
if( BUILD_TESTING )
   add_subdirectory(tests)
endif()
