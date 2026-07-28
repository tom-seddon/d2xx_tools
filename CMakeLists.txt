cmake_minimum_required(VERSION 3.27...3.31)

set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo)
project(d2xx_tools)
set(CMAKE_SKIP_INSTALL_RULES YES)
enable_testing()

# TODO: this should be part of shared_lib
include(CheckCSourceCompiles)

# Always link with libcmt, since that's what the d2xx lib links with.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")#$<$<CONFIG:Debug>:DebugDLL>")


add_subdirectory(submodules)
add_subdirectory(dependencies)
add_subdirectory(src)
