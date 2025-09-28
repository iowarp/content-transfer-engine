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
    
    # Convert the CMAKE_PREFIX_PATH environment variable to a CMake list and prepend to CMAKE_PREFIX_PATH
    if(DEFINED ENV{CMAKE_PREFIX_PATH})
        # Convert colon-separated path to semicolon-separated list for CMake
        string(REPLACE ":" ";" ENV_CMAKE_PREFIX_PATH "$ENV{CMAKE_PREFIX_PATH}")
        # Prepend to existing CMAKE_PREFIX_PATH
        list(PREPEND CMAKE_PREFIX_PATH ${ENV_CMAKE_PREFIX_PATH})
        message(STATUS "CMAKE_PREFIX_PATH set to: ${CMAKE_PREFIX_PATH}")
    endif() 
endif()

# Find required Chimaera framework packages following MODULE_DEVELOPMENT_GUIDE.md patterns
# For internal ChiMod development within the main repository, we need the core framework
find_package(chimaera REQUIRED)              # Core Chimaera (automatically includes ChimaeraCommon.cmake)
find_package(chimaera_admin REQUIRED)        # Admin ChiMod (often required for ChiMod operations)

message(STATUS "Found Chimaera packages:")
message(STATUS "  chimaera: ${chimaera_FOUND}")
message(STATUS "  chimaera_admin: ${chimaera_admin_FOUND}")

# Add nanobind for Python bindings
find_first_path_python()

# Force nanobind to use the Python found by find_first_path_python
set(Python_EXECUTABLE ${PYTHON_SCAN} CACHE FILEPATH "Python executable" FORCE)
set(Python3_EXECUTABLE ${PYTHON_SCAN} CACHE FILEPATH "Python3 executable" FORCE)
message(STATUS "Forcing nanobind to use Python: ${PYTHON_SCAN}")

add_subdirectory(external/nanobind)


message(STATUS "Found Chimaera framework - building full ChiMod implementation")

# Add the core ChiMod
add_subdirectory(core)

# Add the bdev ChiMod (when it exists)
# add_subdirectory(chimods/bdev)

# Add adapters
add_subdirectory(adapter)

# Enable testing
include(CTest)

# Add tests (skip external tests during internal development)
add_subdirectory(test)

# Add wrapper components (includes Python bindings if enabled)
add_subdirectory(wrapper)

message(STATUS "CTE Core and Bdev ChiMods configured for compilation")