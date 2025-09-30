# ------------------------------------------------------------------------------
# Build hermes_core Task Library
# ------------------------------------------------------------------------------
add_chimod_runtime_lib(${REPO_NAMESPACE} hermes_core hermes_core_runtime.cc)
add_chimod_client_lib(${REPO_NAMESPACE} hermes_core hermes_core_client.cc)

if(HERMES_ENABLE_NVIDIA_GDS_ADAPTER)
    target_compile_definitions(hermes_hermes_core PUBLIC HERMES_ENABLE_NVIDIA_GDS_ADAPTER)
    target_link_libraries(hermes_hermes_core PUBLIC cufile)
endif()

if(HERMES_ENABLE_CUDA)
    hshm_enable_cuda(17)
endif()

# ------------------------------------------------------------------------------
# Install hermes_core Task Library
# ------------------------------------------------------------------------------
install(
    TARGETS
    ${hermes_hermes_core_exports}
    EXPORT
    ${CHIMAERA_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${HERMES_INSTALL_LIB_DIR}
    ARCHIVE DESTINATION ${HERMES_INSTALL_LIB_DIR}
    RUNTIME DESTINATION ${HERMES_INSTALL_BIN_DIR}
)

# install(FILES hermes_core_monitor.py
# DESTINATION ${HERMES_INSTALL_LIB_DIR})

# ------------------------------------------------------------------------------
# Coverage
# ------------------------------------------------------------------------------
if(CHIMAERA_ENABLE_COVERAGE)
    set_coverage_flags(hermes_core)
endif()
