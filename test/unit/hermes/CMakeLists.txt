cmake_minimum_required(VERSION 3.10)
project(hermes)

set(CMAKE_CXX_STANDARD 17)

# ------------------------------------------------------------------------------
# Build Tests
# ------------------------------------------------------------------------------
set(TARGETS test_hermes_exec)
add_executable(test_hermes_exec
        ${TEST_MAIN}/main_mpi.cc
        test_init.cc
        test_bucket.cc
)

if(HERMES_ENABLE_CUDA)
        add_cuda_executable(test_bucket_cuda TRUE test_bucket_cuda.cc)
        target_link_libraries(test_bucket_cuda PUBLIC
                hermes_hermes_core_client_gpu MPI::MPI_CXX)
        list(APPEND TARGETS test_bucket_cuda)
endif()

add_dependencies(test_hermes_exec
        ${Hermes_CLIENT_DEPS})
target_link_libraries(test_hermes_exec PUBLIC
        ${Hermes_CLIENT_DEPS} Catch2::Catch2 MPI::MPI_CXX)
jarvis_test(hermes test_hermes)

# ------------------------------------------------------------------------------
# Install Targets
# ------------------------------------------------------------------------------
install(TARGETS
        ${TARGETS}
        LIBRARY DESTINATION ${HERMES_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${HERMES_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${HERMES_INSTALL_BIN_DIR})

# -----------------------------------------------------------------------------
# Coverage
# -----------------------------------------------------------------------------
if(HERMES_ENABLE_COVERAGE)
        set_coverage_flags(test_hermes_exec)
endif()
