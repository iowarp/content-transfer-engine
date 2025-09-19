#------------------------------------------------------------------------------
# Build adapters
#------------------------------------------------------------------------------

# Core adapters that are always built
add_subdirectory(filesystem)
add_subdirectory(posix)

# Optional adapters - only build if explicitly enabled
# These are currently disabled for CTE as they may need additional updates
option(HERMES_ENABLE_STDIO_ADAPTER "Enable STDIO adapter" OFF)
option(HERMES_ENABLE_MPIIO_ADAPTER "Enable MPI-IO adapter" OFF)
option(HERMES_ENABLE_VFD "Enable HDF5 VFD adapter" OFF)
option(HERMES_ENABLE_NVIDIA_GDS_ADAPTER "Enable NVIDIA GDS adapter" OFF)

if (HERMES_ENABLE_STDIO_ADAPTER)
    add_subdirectory(stdio)
endif()
if (HERMES_ENABLE_MPIIO_ADAPTER)
    add_subdirectory(mpiio)
endif()
if (HERMES_ENABLE_VFD)
    add_subdirectory(vfd)
endif()
if (HERMES_ENABLE_NVIDIA_GDS_ADAPTER)
    add_subdirectory(nvidia_gds)
endif()

#-----------------------------------------------------------------------------
# Install adapter headers
#-----------------------------------------------------------------------------
file(GLOB_RECURSE HERMES_HEADERS "*.h")
install(
        FILES
        ${HERMES_HEADERS}
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/include/hermes_adapters
        COMPONENT
        headers
)
install(DIRECTORY mapper DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hermes_adapters)