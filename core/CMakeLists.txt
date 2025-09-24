set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.10)

# Binary output directory is configured in parent CMakeLists.txt to use build/bin
# All .so, .a, and executable files will be placed in build/bin directory
# The global CMAKE_RUNTIME_OUTPUT_DIRECTORY, CMAKE_LIBRARY_OUTPUT_DIRECTORY, 
# and CMAKE_ARCHIVE_OUTPUT_DIRECTORY variables are set in the parent CMakeLists.txt

# Find yaml-cpp for configuration management
find_package(yaml-cpp REQUIRED)

# Create both client and runtime libraries for CTE core module
# This creates targets: wrp_cte_core_runtime and wrp_cte_core_client
add_chimod_both(
  CHIMOD_NAME core
  RUNTIME_SOURCES 
    src/core_runtime.cc
    src/core_config.cc
    src/core_dpe.cc
    src/autogen/core_lib_exec.cc
  CLIENT_SOURCES 
    src/core_client.cc
    src/content_transfer_engine.cc
    src/tag.cc
)

# Link yaml-cpp to the runtime library
target_link_libraries(wrp_cte_core_runtime PRIVATE yaml-cpp)

# Note: bdev functionality should be available through chimaera-core

# Install the ChiMod
# Automatically finds and installs the targets created above
install_chimod(
  CHIMOD_NAME core
)