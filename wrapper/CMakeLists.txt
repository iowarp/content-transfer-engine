# Enable Python bindings by default
option(WRP_CTE_ENABLE_PYTHON "Enable Python bindings" ON)

if(WRP_CTE_ENABLE_PYTHON)
    message("Python bindings enabled")
    add_subdirectory(python)
endif()
