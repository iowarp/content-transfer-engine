cmake_minimum_required(VERSION 3.10)

# Create runtime library using modern ChiMod build functions
add_chimod_runtime(
  CHIMOD_NAME core
  SOURCES 
    src/core_runtime.cc
    src/core_config.cc
    src/core_dpe.cc
    src/autogen/core_lib_exec.cc
)

# Create client library using modern ChiMod build functions
add_chimod_client(
  CHIMOD_NAME core
  SOURCES
    src/core_client.cc
    src/content_transfer_engine.cc
    src/tag.cc
)

# Add utility executable for launching CTE
add_executable(launch_cte util/launch_cte.cc)
target_link_libraries(launch_cte PRIVATE wrp_cte::core_client)

# Install the utility
install(TARGETS launch_cte
  RUNTIME DESTINATION bin
)