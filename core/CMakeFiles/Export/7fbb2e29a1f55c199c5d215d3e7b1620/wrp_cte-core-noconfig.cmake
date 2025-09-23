#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "wrp_cte::wrp_cte_core_runtime" for configuration ""
set_property(TARGET wrp_cte::wrp_cte_core_runtime APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(wrp_cte::wrp_cte_core_runtime PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_NOCONFIG "yaml-cpp"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libwrp_cte_core_runtime.so"
  IMPORTED_SONAME_NOCONFIG "libwrp_cte_core_runtime.so"
  )

list(APPEND _cmake_import_check_targets wrp_cte::wrp_cte_core_runtime )
list(APPEND _cmake_import_check_files_for_wrp_cte::wrp_cte_core_runtime "${_IMPORT_PREFIX}/lib/libwrp_cte_core_runtime.so" )

# Import target "wrp_cte::wrp_cte_core_client" for configuration ""
set_property(TARGET wrp_cte::wrp_cte_core_client APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(wrp_cte::wrp_cte_core_client PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libwrp_cte_core_client.so"
  IMPORTED_SONAME_NOCONFIG "libwrp_cte_core_client.so"
  )

list(APPEND _cmake_import_check_targets wrp_cte::wrp_cte_core_client )
list(APPEND _cmake_import_check_files_for_wrp_cte::wrp_cte_core_client "${_IMPORT_PREFIX}/lib/libwrp_cte_core_client.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
