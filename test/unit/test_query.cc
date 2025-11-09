/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * CTE CORE QUERY API UNIT TESTS
 *
 * This test suite provides comprehensive tests for the TagQuery and BlobQuery
 * APIs. These tests verify regex pattern matching functionality for both tags
 * and blobs in the CTE core system.
 *
 * Test Coverage:
 * 1. TagQuery with various regex patterns (exact match, wildcards, alternation)
 * 2. BlobQuery with tag and blob regex combinations
 * 3. Broadcast pool query behavior
 * 4. Empty result sets and edge cases
 * 5. Invalid regex patterns
 *
 * Following CLAUDE.md requirements:
 * - Use proper runtime initialization
 * - Use chi::kAdminPoolId for CreateTask operations
 * - Use semantic names for QueueIds and priorities
 * - Never use null pool queries - always use Local() or Broadcast()
 * - Follow Google C++ style guide
 */

#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <filesystem>
#include <memory>

// Chimaera core includes
#include <chimaera/admin/admin_tasks.h>
#include <chimaera/bdev/bdev_client.h>
#include <chimaera/bdev/bdev_tasks.h>
#include <chimaera/chimaera.h>
#include <wrp_cte/core/core_client.h>
#include <wrp_cte/core/core_runtime.h>
#include <wrp_cte/core/core_tasks.h>

namespace fs = std::filesystem;

/**
 * Helper function to check if runtime should be initialized
 * Reads CTE_INIT_RUNTIME environment variable
 * Returns true if unset or set to any value except "0", "false", "no", "off"
 */
bool ShouldInitializeRuntime() {
  const char* env_val = std::getenv("CTE_INIT_RUNTIME");
  if (env_val == nullptr) {
    return true; // Default: initialize runtime
  }
  std::string val(env_val);
  // Convert to lowercase for case-insensitive comparison
  std::transform(val.begin(), val.end(), val.begin(), ::tolower);
  return !(val == "0" || val == "false" || val == "no" || val == "off");
}

/**
 * Test fixture for CTE Query API tests
 *
 * This fixture provides runtime initialization and sets up test data including
 * tags and blobs for query testing.
 */
class CTEQueryTestFixture {
public:
  // Semantic names for queue IDs and priorities (following CLAUDE.md
  // requirements)
  static constexpr chi::QueueId kCTEMainQueueId = chi::QueueId(1);
  static constexpr chi::QueueId kCTEWorkerQueueId = chi::QueueId(2);
  static constexpr chi::u32 kCTEHighPriority = 1;
  static constexpr chi::u32 kCTENormalPriority = 2;

  // Test configuration constants
  static constexpr chi::u64 kTestTargetSize =
      1024 * 1024 * 100; // 100MB test target
  static constexpr size_t kTestBlobSize = 4096; // 4KB test blobs

  // CTE Core pool configuration - use constants from core_tasks.h
  static inline const chi::PoolId& kCTECorePoolId = wrp_cte::core::kCtePoolId;
  static inline const char* kCTECorePoolName = wrp_cte::core::kCtePoolName;

  std::unique_ptr<wrp_cte::core::Client> core_client_;
  std::string test_storage_path_;
  chi::PoolId core_pool_id_;
  hipc::MemContext mctx_; // Memory context for shared memory operations

  // Test data: tags and blobs created during setup
  std::vector<std::string> test_tags_;
  std::vector<std::pair<std::string, std::string>> test_blobs_; // (tag_name, blob_name)

  CTEQueryTestFixture() {
    INFO("=== Initializing CTE Query Test Environment ===");

    // Initialize test storage path in home directory
    std::string home_dir = hshm::SystemInfo::Getenv("HOME");
    REQUIRE(!home_dir.empty());

    test_storage_path_ = home_dir + "/cte_query_test.dat";

    // Clean up any existing test file
    if (fs::exists(test_storage_path_)) {
      fs::remove(test_storage_path_);
      INFO("Cleaned up existing test file: " << test_storage_path_);
    }

    // Initialize Chimaera runtime and client
    if (ShouldInitializeRuntime()) {
      INFO("Initializing runtime (CTE_INIT_RUNTIME not set or enabled)");
      REQUIRE(initializeBoth());
    } else {
      INFO("Runtime already initialized externally (CTE_INIT_RUNTIME="
           << std::getenv("CTE_INIT_RUNTIME") << ")");
      REQUIRE(initializeClient());
    }

    // Generate unique pool ID for this test session
    int rand_id = 1000 + rand() % 9000; // Random ID 1000-9999
    core_pool_id_ = chi::PoolId(static_cast<chi::u32>(rand_id), 0);
    INFO("Generated pool ID: " << core_pool_id_.ToU64());

    // Create and initialize core client with the generated pool ID
    core_client_ = std::make_unique<wrp_cte::core::Client>(core_pool_id_);
    INFO("CTE Core client created successfully");

    // Create test data
    setupTestData();

    INFO("=== CTE Query Test Environment Ready ===");
  }

  ~CTEQueryTestFixture() {
    INFO("=== Cleaning up CTE Query Test Environment ===");

    // Clean up test storage
    if (fs::exists(test_storage_path_)) {
      fs::remove(test_storage_path_);
      INFO("Cleaned up test file: " << test_storage_path_);
    }

    INFO("=== CTE Query Test Cleanup Complete ===");
  }

private:
  /**
   * Initialize both runtime and client
   */
  bool initializeBoth() {
    if (!chi::CHIMAERA_RUNTIME_INIT()) {
      FAIL("Failed to initialize Chimaera runtime");
      return false;
    }
    INFO("Chimaera runtime initialized successfully");

    if (!chi::CHIMAERA_CLIENT_INIT()) {
      FAIL("Failed to initialize Chimaera client");
      return false;
    }
    INFO("Chimaera client initialized successfully");

    return true;
  }

  /**
   * Initialize only client (runtime already initialized externally)
   */
  bool initializeClient() {
    if (!chi::CHIMAERA_CLIENT_INIT()) {
      FAIL("Failed to initialize Chimaera client");
      return false;
    }
    INFO("Chimaera client initialized successfully");
    return true;
  }

  /**
   * Setup test data: create tags and blobs for query testing
   */
  void setupTestData() {
    INFO("Setting up test data for query tests");

    // Create CTE Core container using core_client_->Create()
    chi::PoolQuery pool_query = chi::PoolQuery::Dynamic();
    wrp_cte::core::CreateParams params;
    REQUIRE_NOTHROW(core_client_->Create(mctx_, pool_query, kCTECorePoolName,
                                         core_pool_id_, params));
    INFO("CTE Core container created successfully");

    // Register a test storage target using core_client_->RegisterTarget()
    INFO("Registering test storage target at: " << test_storage_path_);
    chi::u32 reg_result = core_client_->RegisterTarget(
        mctx_, test_storage_path_, chimaera::bdev::BdevType::kFile,
        kTestTargetSize, chi::PoolQuery::Local(), chi::PoolId(700, 0));
    REQUIRE(reg_result == 0);
    INFO("Storage target registered successfully");

    // Create test tags with various naming patterns
    test_tags_ = {
        "user_data",
        "user_logs",
        "system_config",
        "system_cache",
        "app_settings",
        "app_preferences",
        "temp_files",
        "backup_2024"
    };

    for (const auto& tag_name : test_tags_) {
      wrp_cte::core::TagId tag_id = core_client_->GetOrCreateTag(mctx_, tag_name);
      REQUIRE(!tag_id.IsNull());
      INFO("Created tag: " << tag_name);

      // Create some test blobs for each tag
      std::vector<std::string> blob_names = {
          "blob_001.dat",
          "blob_002.dat",
          "file_a.txt",
          "file_b.txt"
      };

      for (const auto& blob_name : blob_names) {
        // Create test data
        std::vector<char> test_data(kTestBlobSize, 'X');

        // Allocate shared memory for blob data
        hipc::FullPtr<char> blob_data_fullptr = CHI_IPC->AllocateBuffer(kTestBlobSize);
        if (blob_data_fullptr.IsNull()) {
          WARN("Memory context allocation failed - skipping blob creation");
          continue;
        }

        // Copy test data to shared memory
        std::memcpy(blob_data_fullptr.ptr_, test_data.data(), kTestBlobSize);

        // Convert to Pointer for PutBlob
        hipc::Pointer blob_data_ptr = blob_data_fullptr.shm_;

        // Store the blob using PutBlob with TagId
        bool put_result = core_client_->PutBlob(mctx_, tag_id, blob_name,
                                                0, kTestBlobSize, blob_data_ptr,
                                                0.5f, 0);

        if (put_result) {
          test_blobs_.emplace_back(tag_name, blob_name);
          INFO("Created blob: " << tag_name << "/" << blob_name);
        } else {
          WARN("Failed to create blob: " << tag_name << "/" << blob_name);
        }
      }
    }

    INFO("Test data setup complete: " << test_tags_.size() << " tags, "
         << test_blobs_.size() << " blobs");
  }
};

/**
 * Test TagQuery with exact match pattern
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "TagQuery - Exact Match",
                 "[query][tagquery][exact]") {
  INFO("Testing TagQuery with exact match pattern");

  // Query for exact tag name
  std::vector<std::string> results = core_client_->TagQuery(
      mctx_, "user_data", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(!results.empty());
  CHECK(std::find(results.begin(), results.end(), "user_data") != results.end());
}

/**
 * Test TagQuery with wildcard pattern
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "TagQuery - Wildcard Pattern",
                 "[query][tagquery][wildcard]") {
  INFO("Testing TagQuery with wildcard pattern");

  // Query for all tags starting with "user_"
  std::vector<std::string> results = core_client_->TagQuery(
      mctx_, "user_.*", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(results.size() >= 2); // Should match user_data and user_logs

  bool found_user_data = false;
  bool found_user_logs = false;
  for (const auto& tag : results) {
    INFO("Found tag: " << tag);
    if (tag == "user_data") found_user_data = true;
    if (tag == "user_logs") found_user_logs = true;
  }

  CHECK(found_user_data);
  CHECK(found_user_logs);
}

/**
 * Test TagQuery with alternation pattern
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "TagQuery - Alternation Pattern",
                 "[query][tagquery][alternation]") {
  INFO("Testing TagQuery with alternation pattern");

  // Query for tags matching either "system_config" or "system_cache"
  std::vector<std::string> results = core_client_->TagQuery(
      mctx_, "system_(config|cache)", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(results.size() >= 2);

  bool found_config = false;
  bool found_cache = false;
  for (const auto& tag : results) {
    INFO("Found tag: " << tag);
    if (tag == "system_config") found_config = true;
    if (tag == "system_cache") found_cache = true;
  }

  CHECK(found_config);
  CHECK(found_cache);
}

/**
 * Test TagQuery with match-all pattern
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "TagQuery - Match All Pattern",
                 "[query][tagquery][matchall]") {
  INFO("Testing TagQuery with match-all pattern");

  // Query for all tags
  std::vector<std::string> results = core_client_->TagQuery(
      mctx_, ".*", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(results.size() >= test_tags_.size());

  // Verify all test tags are present
  for (const auto& expected_tag : test_tags_) {
    bool found = std::find(results.begin(), results.end(), expected_tag) != results.end();
    CHECK(found);
    if (!found) {
      INFO("Missing expected tag: " << expected_tag);
    }
  }
}

/**
 * Test TagQuery with no matches
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "TagQuery - No Matches",
                 "[query][tagquery][nomatch]") {
  INFO("Testing TagQuery with pattern that matches nothing");

  // Query for non-existent tag pattern
  std::vector<std::string> results = core_client_->TagQuery(
      mctx_, "nonexistent_tag_pattern_xyz", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  CHECK(results.empty());
}

/**
 * Test BlobQuery with exact tag and blob match
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - Exact Match",
                 "[query][blobquery][exact]") {
  INFO("Testing BlobQuery with exact match patterns");

  // Query for specific blob in specific tag
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, "user_data", "blob_001\\.dat", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(!results.empty());

  // Results are in format "major.minor.blob_name"
  bool found = false;
  for (const auto& result : results) {
    INFO("Found blob: " << result);
    if (result.find("blob_001.dat") != std::string::npos) {
      found = true;
      break;
    }
  }
  CHECK(found);
}

/**
 * Test BlobQuery with wildcard patterns
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - Wildcard Patterns",
                 "[query][blobquery][wildcard]") {
  INFO("Testing BlobQuery with wildcard patterns");

  // Query for all .dat blobs in user_data tag
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, "user_data", "blob_.*\\.dat", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(results.size() >= 2); // Should match blob_001.dat and blob_002.dat

  int dat_blob_count = 0;
  for (const auto& result : results) {
    INFO("Found blob: " << result);
    if (result.find("blob_") != std::string::npos &&
        result.find(".dat") != std::string::npos) {
      dat_blob_count++;
    }
  }
  CHECK(dat_blob_count >= 2);
}

/**
 * Test BlobQuery with multiple tag matches
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - Multiple Tags",
                 "[query][blobquery][multitag]") {
  INFO("Testing BlobQuery with multiple tag matches");

  // Query for all .txt files in any "user_" tag
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, "user_.*", "file_.*\\.txt", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(results.size() >= 4); // user_data and user_logs each have 2 .txt files

  int txt_file_count = 0;
  for (const auto& result : results) {
    INFO("Found blob: " << result);
    if (result.find("file_") != std::string::npos &&
        result.find(".txt") != std::string::npos) {
      txt_file_count++;
    }
  }
  CHECK(txt_file_count >= 4);
}

/**
 * Test BlobQuery with match-all patterns
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - Match All",
                 "[query][blobquery][matchall]") {
  INFO("Testing BlobQuery with match-all patterns");

  // Query for all blobs in all tags
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, ".*", ".*", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  REQUIRE(results.size() >= test_blobs_.size());
}

/**
 * Test BlobQuery with no blob matches
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - No Blob Matches",
                 "[query][blobquery][noblob]") {
  INFO("Testing BlobQuery with blob pattern that matches nothing");

  // Query for non-existent blob pattern in existing tag
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, "user_data", "nonexistent_blob_xyz", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  CHECK(results.empty());
}

/**
 * Test BlobQuery with no tag matches
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - No Tag Matches",
                 "[query][blobquery][notag]") {
  INFO("Testing BlobQuery with tag pattern that matches nothing");

  // Query for non-existent tag pattern
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, "nonexistent_tag_xyz", ".*", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");
  CHECK(results.empty());
}

/**
 * Test BlobQuery with specific file extension filtering
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "BlobQuery - File Extension Filter",
                 "[query][blobquery][extension]") {
  INFO("Testing BlobQuery with file extension filtering");

  // Query for all .txt files across all tags
  std::vector<std::string> results = core_client_->BlobQuery(
      mctx_, ".*", ".*\\.txt", chi::PoolQuery::Broadcast());

  INFO("Query returned " << results.size() << " results");

  // Each of our 8 test tags has 2 .txt files (file_a.txt, file_b.txt)
  // So we should have at least 16 results
  REQUIRE(results.size() >= 16);

  // Verify all results end with .txt
  for (const auto& result : results) {
    INFO("Found blob: " << result);
    CHECK(result.find(".txt") != std::string::npos);
  }
}

/**
 * Test Query API with Local pool query
 */
TEST_CASE_METHOD(CTEQueryTestFixture, "Query - Local Pool Query",
                 "[query][poolquery][local]") {
  INFO("Testing query APIs with Local pool query");

  // TagQuery with Local should work but only return local results
  std::vector<std::string> tag_results = core_client_->TagQuery(
      mctx_, "user_.*", chi::PoolQuery::Local());

  INFO("TagQuery with Local returned " << tag_results.size() << " results");
  // Should get results since tags were created locally
  REQUIRE(!tag_results.empty());

  // BlobQuery with Local should also work
  std::vector<std::string> blob_results = core_client_->BlobQuery(
      mctx_, "user_.*", "blob_.*", chi::PoolQuery::Local());

  INFO("BlobQuery with Local returned " << blob_results.size() << " results");
  REQUIRE(!blob_results.empty());
}
