cmake_minimum_required(VERSION 3.27...3.31)

set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo)
project(d2xx_tools)
set(CMAKE_SKIP_INSTALL_RULES YES)
enable_testing()

# TODO: this should be part of shared_lib
include(CheckCSourceCompiles)

add_subdirectory(submodules)
add_subdirectory(src)
