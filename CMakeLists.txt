cmake_minimum_required(VERSION 3.20)
project(content_transfer_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE integration and development tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure RPATH settings for better runtime linking
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Default install prefix" FORCE)
endif()

# Platform-specific RPATH configuration
if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib:@loader_path:${CMAKE_INSTALL_PREFIX}/lib")
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN:${CMAKE_INSTALL_PREFIX}/lib")
endif()

# Set all binary output directories to build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# For multi-config generators (Visual Studio, Xcode)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
endforeach()

# Optional: Load .env.cmake if it exists (controlled by CTE_ENABLE_DOTENV option)
option(CTE_ENABLE_DOTENV "Enable loading .env.cmake file if it exists" ON)

if(CTE_ENABLE_DOTENV AND EXISTS "${CMAKE_SOURCE_DIR}/.env.cmake")
    message(STATUS "Loading .env.cmake configuration")
    include("${CMAKE_SOURCE_DIR}/.env.cmake")
endif()

# Require Chimaera framework packages
find_package(chimaera-core REQUIRED)
find_package(chimaera-admin REQUIRED)
# Note: chimaera-bdev functionality is included in chimaera-core

message(STATUS "Found Chimaera framework - building full ChiMod implementation")

# Add the core ChiMod
add_subdirectory(core)

# Add unit tests (optional, controlled by BUILD_TESTING)
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test/unit)
    message(STATUS "CTE unit tests configured for compilation")
endif()

message(STATUS "CTE Core ChiMod configured for compilation")