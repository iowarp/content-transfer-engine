cmake_minimum_required(VERSION 3.20)

# Find Python for nanobind
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)

# Create the Python module
nanobind_add_module(
    _wrp_cte_core_ext
    core_bindings.cpp
)

# Link against the core client library
target_link_libraries(_wrp_cte_core_ext PRIVATE wrp_cte_core_client)

# Include directories for headers
target_include_directories(_wrp_cte_core_ext PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/external/nanobind/include
)

# Ensure C++17 standard
target_compile_features(_wrp_cte_core_ext PRIVATE cxx_std_17)

# Install the module
nanobind_add_stub(
    _wrp_cte_core_ext_stub
    MODULE _wrp_cte_core_ext
    OUTPUT core_stub.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:_wrp_cte_core_ext>
    DEPENDS _wrp_cte_core_ext
)

# Add Python bindings test
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
        add_test(
            NAME python_bindings_test
            COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_python_bindings.py
            WORKING_DIRECTORY $<TARGET_FILE_DIR:_wrp_cte_core_ext>
        )
        
        # Set environment for the test to find the module
        set_tests_properties(python_bindings_test PROPERTIES
            ENVIRONMENT "PYTHONPATH=$<TARGET_FILE_DIR:_wrp_cte_core_ext>"
        )
        
        # Ensure the test depends on the built module
        set_tests_properties(python_bindings_test PROPERTIES
            DEPENDS _wrp_cte_core_ext
        )
    endif()
endif()