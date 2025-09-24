# External CTE Core Integration Test
# This CMakeLists.txt demonstrates how external applications can link to CTE Core
cmake_minimum_required(VERSION 3.16)
project(cte_external_integration_test)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable debug build by default for testing
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Find primary dependencies (same as main CTE project)
find_package(chimaera-core REQUIRED)
find_package(chimaera-admin REQUIRED)
find_package(yaml-cpp REQUIRED)

# Find required system packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Add .env.cmake if it exists (for development environment)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../.env.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/../../../.env.cmake")
endif()

# Set paths to the built CTE Core libraries (assumes they are installed or built)
# In a real external project, these would come from find_package() or pkg-config
set(CTE_CORE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
set(CTE_BUILD_DIR "${CTE_CORE_ROOT}/build")

# Include directories for CTE Core headers
include_directories(
    ${CTE_CORE_ROOT}/core/include
    ${CTE_CORE_ROOT}/adapter
    ${CTE_BUILD_DIR}/core/include  # For any generated headers
)

# Add Chimaera framework paths (would normally be installed system-wide)
# For testing, we assume they're available via spack or similar
pkg_check_modules(CHIMAERA REQUIRED chimaera)
include_directories(${CHIMAERA_INCLUDE_DIRS})
link_directories(${CHIMAERA_LIBRARY_DIRS})

# Add HSHM paths
pkg_check_modules(HSHM REQUIRED hermes_shm)
include_directories(${HSHM_INCLUDE_DIRS})
link_directories(${HSHM_LIBRARY_DIRS})

# Create the external integration test executable
add_executable(cte_external_test
    external_integration_test.cc
)

# Link against CTE Core libraries
# In a real project, these paths would be resolved via find_package()
target_link_libraries(cte_external_test
    # CTE Core libraries (adjust paths as needed)
    ${CTE_BUILD_DIR}/core/libwrp_cte_core_runtime.so
    ${CTE_BUILD_DIR}/core/libwrp_cte_core_client.so
    ${CTE_BUILD_DIR}/adapter/filesystem/libwrp_cte_fs_base.so
    ${CTE_BUILD_DIR}/adapter/libwrp_cte_cae_config.so
    
    # System dependencies
    Threads::Threads
    yaml-cpp
    
    # Chimaera framework dependencies
    ${CHIMAERA_LIBRARIES}
    ${HSHM_LIBRARIES}
)

# Set compile flags
target_compile_options(cte_external_test PRIVATE
    ${CHIMAERA_CFLAGS_OTHER}
    ${HSHM_CFLAGS_OTHER}
    -Wall -Wextra
)

# Set compile definitions
target_compile_definitions(cte_external_test PRIVATE
    ${CHIMAERA_CFLAGS_OTHER}
    ${HSHM_CFLAGS_OTHER}
)

# Add a custom target to build and run the test
add_custom_target(run_external_test
    COMMAND ./cte_external_test
    DEPENDS cte_external_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running external CTE integration test"
)

# Print configuration information
message(STATUS "CTE External Test Configuration:")
message(STATUS "  CTE_CORE_ROOT: ${CTE_CORE_ROOT}")
message(STATUS "  CTE_BUILD_DIR: ${CTE_BUILD_DIR}")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Chimaera Found: ${chimaera-core_FOUND}")
message(STATUS "  yaml-cpp Found: ${yaml-cpp_FOUND}")