project(hermes_external)
cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)

find_package(Hermes REQUIRED)
message("Found hermes at ${HERMES_LIB_DIR}")
add_executable(test_hermes_external_compile
        external.cc
)
target_link_libraries(test_hermes_external_compile
        hermes::hermes_core_client)