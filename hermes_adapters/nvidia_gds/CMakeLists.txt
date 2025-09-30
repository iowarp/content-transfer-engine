include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_cuda_library(hermes_nvidia_gds SHARED TRUE cufile_api.cc)
target_link_libraries(hermes_nvidia_gds PUBLIC hshm::cudacxx MPI::MPI_CXX)

install(TARGETS hermes_nvidia_gds
        LIBRARY DESTINATION ${HERMES_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${HERMES_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${HERMES_INSTALL_BIN_DIR}
        )