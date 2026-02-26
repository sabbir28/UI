cmake_minimum_required(VERSION 3.17)
project(UI)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OUTPUT_BIN_DIR ${CMAKE_BINARY_DIR}/bin)
set(OUTPUT_ASSET_DIR ${OUTPUT_BIN_DIR}/assets)

file(MAKE_DIRECTORY ${OUTPUT_BIN_DIR})
file(MAKE_DIRECTORY ${OUTPUT_ASSET_DIR})

add_library(UIToggle SHARED
    lib/UI/src/Toggle.cpp
)

target_compile_definitions(UIToggle PRIVATE UI_TOGGLE_DLL_EXPORTS)
target_include_directories(UIToggle PUBLIC ${CMAKE_SOURCE_DIR}/lib/UI/include)
target_link_libraries(UIToggle PRIVATE msimg32)

add_executable(UIToggleSample
    main.cpp
)

target_include_directories(UIToggleSample PRIVATE ${CMAKE_SOURCE_DIR}/lib/UI/include)
set_target_properties(UIToggleSample PROPERTIES OUTPUT_NAME "UI")
target_link_libraries(UIToggleSample PRIVATE user32)

set_target_properties(UIToggle UIToggleSample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BIN_DIR}
)

add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/lib/UI/assets
    ${OUTPUT_ASSET_DIR}
)

add_dependencies(UIToggle copy_assets)
add_dependencies(UIToggleSample copy_assets)
