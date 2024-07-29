set(OSC_DISCOVER_TESTS ON
    CACHE BOOL
    "enable/disable automatically running test discovery (IDE integration)"
)

if(${OSC_BUILD_OPENSIMCREATOR})
    add_subdirectory(TestOpenSimCreator)
    add_subdirectory(TestOpenSimThirdPartyPlugins)
endif()
add_subdirectory(testoscar)
add_subdirectory(testoscar_demos)
add_subdirectory(testoscar_learnopengl)
