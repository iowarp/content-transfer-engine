set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tasks/chimaera_admin/include)
add_subdirectory(config)
add_subdirectory(hermes)
add_subdirectory(hermes_adapters)
