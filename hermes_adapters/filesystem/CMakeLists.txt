include_directories(
        ${CMAKE_SOURCE_DIR}
        .)

# Add include directories for hermes dependencies
include_directories($ENV{HOME}/.scspkg/packages/cte-hermes-shm/include)
include_directories($ENV{HOME}/.scspkg/packages/cte/include)

# Create the metadata manager singleton + FS base class
add_library(hermes_fs_base SHARED
        filesystem.cc
        filesystem.h
        filesystem_io_client.h
        filesystem_mdm.h)
# Find the required libraries
find_library(HERMES_SHM_HOST_LIB hermes_shm_host PATHS $ENV{HOME}/.scspkg/packages/cte-hermes-shm/lib)

target_link_libraries(hermes_fs_base
        MPI::MPI_CXX
        wrp_cte_core_client
        ${HERMES_SHM_HOST_LIB})

# -----------------------------------------------------------------------------
# Add Target(s) to CMake Install
# -----------------------------------------------------------------------------
install(
        TARGETS
        hermes_fs_base
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)
install(
        FILES
        filesystem_io_client.h
        DESTINATION
        include/hermes_adapters/filesystem
        COMPONENT
        headers
)

# -----------------------------------------------------------------------------
# Install headers
# -----------------------------------------------------------------------------
file(GLOB_RECURSE HERMES_HEADERS "*.h")
install(
        FILES
        ${HERMES_HEADERS}
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/hermes_adapters/filesystem
        COMPONENT
        headers
)

# -----------------------------------------------------------------------------
# Add Target(s) to Coverage
# -----------------------------------------------------------------------------
# Coverage flags removed for CTE build
