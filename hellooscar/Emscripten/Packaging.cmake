# Emscripten packaging

# for now, just ensure that the html loader is next to the built js+wasm
add_custom_command(
    TARGET hellooscar
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Emscripten/hellooscar.html $<TARGET_FILE_DIR:hellooscar>
    COMMAND_EXPAND_LISTS
)
