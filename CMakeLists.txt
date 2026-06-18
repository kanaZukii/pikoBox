cmake_minimum_required(VERSION 3.16)
project(pikoBox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# RAYLIB OPTIMIZATION FLAGS
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build raylib tests" FORCE)

# Modern Raylib Linux window backend platform flag configurations
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build Raylib with Wayland support" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "Build Raylib with X11 support" FORCE)

# RAYLIB SUBMODULE VERIFICATION
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/raylib/CMakeLists.txt")
    add_subdirectory(extern/raylib)
else()
    message(FATAL_ERROR "Raylib submodule not found! Did you forget to run 'git submodule update --init --recursive'?")
endif()

# COLLECT PIKOBOX SOURCE FILES
file(GLOB_RECURSE ENGINE_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# DEFINE THE STATIC LIBRARY TARGET
add_library(pikoBox STATIC ${ENGINE_SOURCES})

# EXPORT SYSTEM INCLUDE SEARCH PATHS
# Removed $<BUILD_INTERFACE> so downstream projects can actually see your headers!
target_include_directories(pikoBox
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/nlohmann"
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/raylib/src"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# LINK LIBRARIES
# Raylib automatically forwards its platform dependencies (X11/Wayland/Win32) down the chain!
target_link_libraries(pikoBox PUBLIC raylib)

# Output library binary to a standard lib build destination folder
set_target_properties(pikoBox PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/"
)