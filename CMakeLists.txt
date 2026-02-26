cmake_minimum_required(VERSION 3.17)
project(UI)

# Language level (unchanged)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(OUTPUT_BIN_DIR   ${CMAKE_BINARY_DIR}/bin)
set(OUTPUT_DATA_DIR  ${CMAKE_BINARY_DIR}/data)
set(OUTPUT_ASSET_DIR ${OUTPUT_DATA_DIR}/assets)


file(MAKE_DIRECTORY ${OUTPUT_BIN_DIR})
file(MAKE_DIRECTORY ${OUTPUT_ASSET_DIR})

# Executable
add_executable(
        UI
        main.cpp
        lib/UI/src/Toggle.cpp
)

target_link_libraries(UI msimg32)

# Send all binaries to /bin
set_target_properties(UI PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
        LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
        )

# Copy assets to build/data/assets
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/lib/UI/assets
        ${OUTPUT_ASSET_DIR}
        )