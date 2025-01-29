cmake_minimum_required(VERSION 3.10)
project(hermes)

# -----------------------------------------------------------------------------
# Define Constants
# -----------------------------------------------------------------------------
set(HERMES_VERSION_MAJOR 2)
set(HERMES_VERSION_MINOR 0)
set(HERMES_VERSION_PATCH 0)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --diag-suppress=20012")

# ------------------------------------------------------------------------------
# DOTENV
# ------------------------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/.env.cmake)
    include(.env.cmake)
endif()

# -----------------------------------------------------------------------------
# Define Options
# -----------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(BUILD_OpenMP_TESTS "Build tests which depend on OpenMP" ON)
option(HERMES_ENABLE_COVERAGE "Check how well tests cover code" OFF)
option(HERMES_ENABLE_DOXYGEN "Check how well the code is documented" ON)
option(HERMES_REMOTE_DEBUG "Enable remote debug mode on hrun" OFF)

option(HERMES_ENABLE_NVIDIA_GDS_ADAPTER "Build the Hermes NVIDIA GDS adapter." OFF)
option(HERMES_ENABLE_POSIX_ADAPTER "Build the Hermes POSIX adapter." ON)
option(HERMES_ENABLE_STDIO_ADAPTER "Build the Hermes stdio adapter." OFF)
option(HERMES_ENABLE_MPIIO_ADAPTER "Build the Hermes MPI-IO adapter." OFF)
option(HERMES_ENABLE_VFD "Build the Hermes HDF5 Virtual File Driver" OFF)
option(HERMES_ENABLE_PUBSUB_ADAPTER "Build the Hermes pub/sub adapter." OFF)
option(HERMES_ENABLE_KVSTORE "Build the Hermes KVStore adapter." OFF)
option(HERMES_ENABLE_PYTHON "Build the Hermes Python wrapper" OFF)
option(HERMES_ENABLE_ADIOS "Build the Hermes Python wrapper" OFF)

option(HERMES_MPICH "Specify that this a MPICH build" OFF)
option(HERMES_OPENMPI "Specify that this a OpenMPI build" OFF)

option(HERMES_ENABLE_CUDA "Enable CUDA support" OFF)
option(HERMES_ENABLE_ROCM "Enable ROCm support" OFF)

# Ensure CUDA and GDS on at same time
if(HERMES_ENABLE_NVIDIA_GDS_ADAPTER)
    set(HERMES_ENABLE_CUDA ON)
endif()

# A hack for spack to get dependencies
option(HERMES_NO_COMPILE "Don't compile the code" OFF)

if(HERMES_NO_COMPILE)
    install(
        FILES
        ${CMAKE_SOURCE_DIR}/.gitignore
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/.gitignore
    )
    return()
endif()

# -----------------------------------------------------------------------------
# Compiler Optimization
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_compile_definitions(HERMES_LOG_VERBOSITY=10)
else()
    message("IN RELEASE MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")
    add_compile_definitions(HERMES_LOG_VERBOSITY=1)
endif()

add_compile_options(-march=native -fomit-frame-pointer)

# -----------------------------------------------------------------------------
# Targets built within this project are exported at Install time for use
# by other projects.
# -----------------------------------------------------------------------------
if(NOT HERMES_EXPORTED_TARGETS)
    set(HERMES_EXPORTED_TARGETS "Hermes")
endif()

# -----------------------------------------------------------------------------
# Documentation
# -----------------------------------------------------------------------------
# if (HERMES_ENABLE_DOXYGEN)
# include(UseDoxygenDoc)
#
# add_doxygen_doc(
# BUILD_DIR
# ${CMAKE_CURRENT_BINARY_DIR}/_build
# DOXY_FILE
# ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
# TARGET_NAME
# dox
# COMMENT
# "HTML documentation"
# )
# endif()

# -----------------------------------------------------------------------------
# Code Coverage
# -----------------------------------------------------------------------------
if(HERMES_ENABLE_COVERAGE)
    set(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage --coverage" CACHE STRING
        "Flags to the coverage program to perform coverage inspection"
    )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${COVERAGE_FLAGS}")

    mark_as_advanced(COVERAGE_FLAGS)

    macro(set_coverage_flags target)
        target_link_libraries(${target} gcov)

        # set_target_properties(${target}
        # PROPERTIES
        # COMPILE_FLAGS ${COVERAGE_FLAGS}
        # LINK_FLAGS ${COVERAGE_FLAGS}
        # )
    endmacro()
endif()

add_custom_target(coverage COMMAND bash ${CMAKE_SOURCE_DIR}/ci/coverage.sh
    ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR})

# -----------------------------------------------------------------------------
# Find Packages
# -----------------------------------------------------------------------------

# Include most packages needed by both client and host
include(CMake/HermesCommonConfig.cmake)
set(Hermes_CLIENT_DEPS hermes_client)
set(Hermes_RUNTIME_DEPS hermes_runtime)

# ENABLE GPU SUPPORT
if(CHIMAERA_ENABLE_CUDA)
    hshm_enable_cuda(17)
endif()

if(CHIMAERA_ENABLE_ROCM)
    hshm_enable_rocm("HIP" 17)
endif()

# Zeromq
# pkg_check_modules(ZMQ REQUIRED libzmq)
# include_directories(${ZMQ_INCLUDE_DIRS})
# message("Found libzmq at: ${ZMQ_INCLUDE_DIRS}")

# HDF5 (only for VFD, not the basic client)
if(HERMES_ENABLE_VFD)
    set(HERMES_REQUIRED_HDF5_VERSION 1.14.0)
    set(HERMES_REQUIRED_HDF5_COMPONENTS C)
    find_package(HDF5 ${HERMES_REQUIRED_HDF5_VERSION} CONFIG NAMES hdf5
        COMPONENTS ${HERMES_REQUIRED_HDF5_COMPONENTS} shared)

    if(HDF5_FOUND)
        message(STATUS "found HDF5 ${HDF5_VERSION} at ${HDF5_INCLUDE_DIR}")
        set(HDF5_HERMES_VFD_EXT_INCLUDE_DEPENDENCIES
            ${HDF5_HERMES_VFD_EXT_INCLUDE_DEPENDENCIES}
            ${HDF5_INCLUDE_DIR})
        set(HDF5_HERMES_VFD_EXT_LIB_DEPENDENCIES
            ${HDF5_HERMES_VFD_EXT_LIB_DEPENDENCIES}
            ${HDF5_C_SHARED_LIBRARY})
    else()
        # Allow for HDF5 autotools builds
        find_package(HDF5 ${HERMES_REQUIRED_HDF5_VERSION} MODULE REQUIRED
            COMPONENTS ${HERMES_REQUIRED_HDF5_COMPONENTS})

        if(HDF5_FOUND)
            set(HDF5_HERMES_VFD_EXT_INCLUDE_DEPENDENCIES
                ${HDF5_HERMES_VFD_EXT_INCLUDE_DEPENDENCIES}
                ${HDF5_INCLUDE_DIRS})
            set(HDF5_HERMES_VFD_EXT_LIB_DEPENDENCIES
                ${HDF5_HERMES_VFD_EXT_LIB_DEPENDENCIES}
                ${HDF5_LIBRARIES})
        else()
            message(FATAL_ERROR "Could not find HDF5, please set HDF5_DIR (1.13.0) or HDF5_ROOT (1.13.1).")
        endif()
    endif()
endif()

# Dependencies for the client
add_library(client_deps INTERFACE)
target_link_libraries(client_deps INTERFACE
    chimaera::client_host)
target_include_directories(client_deps INTERFACE ${libelf_INCLUDE_DIRS})
target_link_directories(client_deps INTERFACE ${libelf_LIBRARY_DIRS})
target_link_libraries(client_deps INTERFACE ${libelf_LIBRARIES})

# Dependencies for the runtime
add_library(runtime_deps INTERFACE)
target_link_libraries(runtime_deps INTERFACE
    chimaera::runtime)
target_include_directories(runtime_deps INTERFACE ${libelf_INCLUDE_DIRS})
target_link_directories(runtime_deps INTERFACE ${libelf_LIBRARY_DIRS})
target_link_libraries(runtime_deps INTERFACE ${libelf_LIBRARIES})

# ------------------------------------------------------------------------------
# Setup CMake Environment
# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries.")

# ------------------------------------------------------------------------------
# Setup install and output Directories
# ------------------------------------------------------------------------------
if(NOT HERMES_INSTALL_BIN_DIR)
    set(HERMES_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()

if(NOT HERMES_INSTALL_LIB_DIR)
    set(HERMES_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if(NOT HERMES_INSTALL_INCLUDE_DIR)
    set(HERMES_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()

if(NOT HERMES_INSTALL_DATA_DIR)
    set(HERMES_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

# -----------------------------------------------------------------------------
# Build Hermes Main Packages
# -----------------------------------------------------------------------------
# Main includes
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${HermesShm_LIBRARY_DIRS})

# Task includes
include_directories(${CMAKE_SOURCE_DIR}/tasks)
include_directories(${CMAKE_SOURCE_DIR}/tasks/hermes_core/include)

# Test includes
include_directories(${CMAKE_SOURCE_DIR}/test/unit)

set(TEST_MAIN ${CMAKE_SOURCE_DIR}/test/unit)
add_subdirectory(src)
add_subdirectory(hermes_adapters)
add_subdirectory(tasks)

# add_subdirectory(benchmark)
# add_subdirectory(wrapper)
add_custom_target(lint COMMAND bash ${CMAKE_SOURCE_DIR}/ci/lint.sh ${CMAKE_SOURCE_DIR})

# -----------------------------------------------------------------------------
# Build + Enable Testing
# -----------------------------------------------------------------------------
# General function used to hook ctest to python test tool lib
function(pytest test_type test_name)
    set(script ${CMAKE_SOURCE_DIR}/scripts/ci/py_hermes_ci/bin/run_test)

    # add_test(NAME ${test_name}
    # COMMAND ${script} ${test_type} ${test_name} ${CMAKE_BINARY_DIR} ${HERMES_USE_ADDRESS_SANITIZER})
endfunction()

function(jarvis_test test_type test_name)
    add_test(NAME ${test_name}
        COMMAND jarvis pipeline run yaml "${CMAKE_SOURCE_DIR}/test/unit/pipelines/${test_type}/${test_name}.yaml")
endfunction()

enable_testing()
add_subdirectory(test)

# -----------------------------------------------------------------------------
# Install Hermes Headers
# -----------------------------------------------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/HermesCommonConfig.cmake
    ${PROJECT_BINARY_DIR}/CMakeFiles/HermesCommonConfig.cmake @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/HermesConfig.cmake
    ${PROJECT_BINARY_DIR}/CMakeFiles/HermesConfig.cmake @ONLY
)

install(
    FILES
    ${PROJECT_BINARY_DIR}/CMakeFiles/HermesCommonConfig.cmake
    ${PROJECT_BINARY_DIR}/CMakeFiles/HermesConfig.cmake
    DESTINATION
    ${CMAKE_INSTALL_PREFIX}/cmake
)

install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
install(EXPORT ${HERMES_EXPORTED_TARGETS}
    FILE ${HERMES_EXPORTED_TARGETS}CoreConfig.cmake
    NAMESPACE Hermes::
    DESTINATION cmake
)