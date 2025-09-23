# CMake generated Testfile for 
# Source directory: /mnt/home/Projects/iowarp/content-transfer-engine/test/unit
# Build directory: /mnt/home/Projects/iowarp/content-transfer-engine/test/unit
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(cte_core_pool_creation "/mnt/home/Projects/iowarp/content-transfer-engine/bin/cte_core_unit_tests" "[cte][core][pool][creation]")
set_tests_properties(cte_core_pool_creation PROPERTIES  LABELS "unit;core;cte" TIMEOUT "300" _BACKTRACE_TRIPLES "/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;52;add_test;/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;0;")
add_test(cte_core_target_registration "/mnt/home/Projects/iowarp/content-transfer-engine/bin/cte_core_unit_tests" "[cte][core][target][registration]")
set_tests_properties(cte_core_target_registration PROPERTIES  LABELS "unit;core;cte" TIMEOUT "300" _BACKTRACE_TRIPLES "/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;55;add_test;/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;0;")
add_test(cte_core_blob_put "/mnt/home/Projects/iowarp/content-transfer-engine/bin/cte_core_unit_tests" "[cte][core][blob][put]")
set_tests_properties(cte_core_blob_put PROPERTIES  LABELS "unit;core;cte" TIMEOUT "300" _BACKTRACE_TRIPLES "/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;58;add_test;/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;0;")
add_test(cte_core_blob_get "/mnt/home/Projects/iowarp/content-transfer-engine/bin/cte_core_unit_tests" "[cte][core][blob][get]")
set_tests_properties(cte_core_blob_get PROPERTIES  LABELS "unit;core;cte" TIMEOUT "300" _BACKTRACE_TRIPLES "/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;61;add_test;/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;0;")
add_test(cte_core_integration "/mnt/home/Projects/iowarp/content-transfer-engine/bin/cte_core_unit_tests" "[cte][core][integration]")
set_tests_properties(cte_core_integration PROPERTIES  LABELS "unit;core;cte" TIMEOUT "300" _BACKTRACE_TRIPLES "/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;64;add_test;/mnt/home/Projects/iowarp/content-transfer-engine/test/unit/CMakeLists.txt;0;")
subdirs("adapters")
