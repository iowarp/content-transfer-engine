# ------------------------------------------------------------------------------
# CTE Core Unit Tests
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)

# Note: Binary output directory is configured in root CMakeLists.txt to use build/bin
# The test executable will be placed in build/bin directory due to global
# CMAKE_RUNTIME_OUTPUT_DIRECTORY setting

# Find required test dependencies
find_package(Catch2 REQUIRED)

# Create the test executable
add_executable(cte_core_unit_tests
    test_core_minimal.cc
    test_core_functionality.cc
    test_core_functionality_simple.cc
)

# Link against required libraries using modern target names
target_link_libraries(cte_core_unit_tests
    wrp_cte_core_client          # CTE core client library
    wrp_cte_core_runtime         # CTE core runtime library  
    chimaera::cxx                # Modern Chimaera core framework target
    chimaera::admin_client       # Modern Chimaera admin target (if needed)
    Catch2::Catch2WithMain       # Catch2 testing framework with main
    MPI::MPI_CXX                 # MPI support if needed
)

# Set compilation standards and warnings
target_compile_features(cte_core_unit_tests PRIVATE cxx_std_17)
target_compile_options(cte_core_unit_tests PRIVATE
    -Wall -Wextra -Werror        # Strict warnings as per CLAUDE.md
    $<$<CONFIG:Debug>:-g -O0>    # Debug symbols and no optimization for debug
    $<$<CONFIG:Release>:-O3>     # Optimization for release
)

# Include directories
target_include_directories(cte_core_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/core/src
)

# Add test discovery for CTest integration
include(CTest)
if(BUILD_TESTING)
    # Note: Manual test registration is used below instead of auto-discovery
    # to ensure compatibility with the current Catch2 setup
endif()

# Manual test registration for specific test cases
add_test(NAME cte_core_pool_creation 
    COMMAND cte_core_unit_tests "[cte][core][pool][creation]")
    
add_test(NAME cte_core_target_registration 
    COMMAND cte_core_unit_tests "[cte][core][target][registration]")
    
add_test(NAME cte_core_blob_put 
    COMMAND cte_core_unit_tests "[cte][core][blob][put]")
    
add_test(NAME cte_core_blob_get 
    COMMAND cte_core_unit_tests "[cte][core][blob][get]")
    
add_test(NAME cte_core_integration 
    COMMAND cte_core_unit_tests "[cte][core][integration]")

# Set test properties for proper execution environment
set_tests_properties(
    cte_core_pool_creation
    cte_core_target_registration
    cte_core_blob_put
    cte_core_blob_get
    cte_core_integration
    PROPERTIES
        TIMEOUT 300  # 5 minute timeout for each test
        LABELS "unit;core;cte"
)

# ------------------------------------------------------------------------------
# Coverage Support
# ------------------------------------------------------------------------------
if(WRP_CTE_ENABLE_COVERAGE)
    set_coverage_flags(cte_core_unit_tests)
endif()

# ------------------------------------------------------------------------------
# Install Targets
# ------------------------------------------------------------------------------
install(TARGETS cte_core_unit_tests
    LIBRARY DESTINATION ${HERMES_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${HERMES_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${HERMES_INSTALL_BIN_DIR}
)

# ------------------------------------------------------------------------------
# Add External Integration Tests
# ------------------------------------------------------------------------------
# External test demonstrates proper external linking patterns
# Temporarily commented out until ChiMod is installed and can be found as a package
# add_subdirectory(external)

# ------------------------------------------------------------------------------
# Add Adapter Unit Tests
# ------------------------------------------------------------------------------
add_subdirectory(adapters)