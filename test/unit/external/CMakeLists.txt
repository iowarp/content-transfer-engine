# External CTE Core Integration Test
# This CMakeLists.txt demonstrates how external applications can link to CTE Core
# following the MODULE_DEVELOPMENT_GUIDE.md patterns
cmake_minimum_required(VERSION 3.20)
project(cte_external_integration_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug build by default for testing
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Add .env.cmake if it exists (for development environment)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../.env.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../../.env.cmake")
endif()

# Find required Chimaera framework packages - core dependencies
find_package(chimaera REQUIRED)              # Core Chimaera library (libcxx.so)
find_package(chimaera_admin REQUIRED)        # Admin ChiMod (required for most applications)

# Find CTE Core ChiMod package (main target for external applications)
find_package(wrp_cte_core REQUIRED)          # CTE Core ChiMod package

# Find additional system dependencies
find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Create the external integration test executable
add_executable(cte_external_test
    external_integration_test.cc
)

# Link against CTE Core libraries using modern CMake target naming
# These targets are created by the add_chimod_both() function in the core ChiMod
target_link_libraries(cte_external_test
    # CTE Core ChiMod libraries (from find_package(wrp_cte_core))
    wrp_cte::core_client                 # CTE Core client library
    wrp_cte::core_runtime                # CTE Core runtime library (if needed)
    
    # Core Chimaera framework (automatically included by ChiMod libraries)
    # chimaera::cxx                      # NOT needed - automatically included
    
    # Admin ChiMod (if needed for advanced functionality)
    # chimaera::admin_client             # Optional - uncomment if needed
)

# Set compile flags - use standard flags for external projects
target_compile_options(cte_external_test PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)

# Add a custom target to build and run the test
add_custom_target(run_external_test
    COMMAND ./cte_external_test
    DEPENDS cte_external_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running external CTE integration test"
)

# Print configuration information for external project setup verification
message(STATUS "CTE External Integration Test Configuration:")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Chimaera Core Found: ${chimaera_FOUND}")
message(STATUS "  Chimaera Admin Found: ${chimaera_admin_FOUND}")
message(STATUS "  CTE Core Found: ${wrp_cte_core_FOUND}")
message(STATUS "  yaml-cpp Found: ${yaml-cpp_FOUND}")

# Instructions for external developers
message(STATUS "")
message(STATUS "External Developer Notes:")
message(STATUS "  This test demonstrates proper external linking to CTE Core libraries.")
message(STATUS "  Required packages: chimaera, chimaera_admin, wrp_cte_core")
message(STATUS "  Main target: wrp_cte::core_client (ChiMod client library)")
message(STATUS "  Optional: wrp_cte::core_runtime (ChiMod runtime library)")
message(STATUS "  Framework dependencies are automatically included by ChiMod targets.")
message(STATUS "")