include_directories(
        ${CMAKE_SOURCE_DIR}
        .)

# Add include directories for hermes dependencies
include_directories($ENV{HOME}/.scspkg/packages/cte-hermes-shm/include)
include_directories($ENV{HOME}/.scspkg/packages/cte/include)

# Create the POSIX interceptor
add_library(hermes_posix SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/posix_api.cc)
add_dependencies(hermes_posix hermes_fs_base)
# Find the required libraries
find_library(HERMES_SHM_HOST_LIB hermes_shm_host PATHS $ENV{HOME}/.scspkg/packages/cte-hermes-shm/lib)

target_link_libraries(hermes_posix
        MPI::MPI_CXX
        stdc++fs
        hermes_fs_base
        wrp_cte_core_client
        ${HERMES_SHM_HOST_LIB})
set_target_properties(hermes_posix PROPERTIES POSITION_INDEPENDENT_CODE ON)


# -----------------------------------------------------------------------------
# Add Target(s) to CMake Install
# -----------------------------------------------------------------------------
install(
        TARGETS
        hermes_posix
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

# -----------------------------------------------------------------------------
# Install headers
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HERMES_HEADERS "*.h")
install(
        FILES
        ${HERMES_HEADERS}
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/hermes_adapters/posix
        COMPONENT
        headers
)

# -----------------------------------------------------------------------------
# Add Target(s) to Coverage
# -----------------------------------------------------------------------------
# Coverage flags removed for CTE build
