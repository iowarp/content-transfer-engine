# Use the Chimaera helper macros to create bdev_extended ChiMod with target registration
# Note: We use 'bdev_extended' as the ChiMod name to avoid conflicts with existing bdev

# Find required dependencies for extended functionality
find_package(Threads REQUIRED)

# Link libaio on Linux systems
if(UNIX AND NOT APPLE)
    find_library(AIO_LIBRARY aio)
endif()

# Create both client and runtime libraries for bdev_extended module
add_chimod_both(
  CHIMOD_NAME bdev_extended
  RUNTIME_SOURCES 
    src/bdev_runtime.cc
    src/autogen/bdev_lib_exec.cc
  # No separate client sources - using header-only client
)

# Link additional libraries to the runtime
if(AIO_LIBRARY)
    target_link_libraries(wrp_cte_bdev_extended_runtime PRIVATE ${AIO_LIBRARY})
    target_compile_definitions(wrp_cte_bdev_extended_runtime PRIVATE CHIMAERA_BDEV_HAS_AIO)
endif()

target_link_libraries(wrp_cte_bdev_extended_runtime PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# Configure RPATH for the runtime target
set_property(TARGET wrp_cte_bdev_extended_runtime PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
if(TARGET wrp_cte_bdev_extended_client)
    set_property(TARGET wrp_cte_bdev_extended_client PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Install the ChiMod
install_chimod(
  CHIMOD_NAME bdev_extended
)