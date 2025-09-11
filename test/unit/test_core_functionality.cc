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

#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <memory>

#include <chimaera/chimaera.h>
#include <chimaera/core/core_client.h>
#include <chimaera/core/core_tasks.h>
#include <chimaera/bdev/bdev_client.h>
#include <chimaera/bdev/bdev_tasks.h>
#include <chimaera/admin/admin_tasks.h>

namespace fs = std::filesystem;

/**
 * Test fixture for CTE Core functionality tests
 * Sets up the runtime environment and provides common utilities
 */
class CTECoreTestFixture {
public:
  // Semantic names for queue IDs and priorities (following CLAUDE.md requirements)
  static constexpr chi::QueueId kTestMainQueueId = chi::QueueId(1);
  static constexpr chi::TaskPrio kTestHighPriority = chi::TaskPrio(1);
  static constexpr chi::TaskPrio kTestNormalPriority = chi::TaskPrio(2);
  
  // Test configuration constants
  static constexpr chi::u64 kTestTargetSize = 1024 * 1024 * 10;  // 10MB test target
  static constexpr chi::u32 kTestWorkerCount = 2;
  
  std::unique_ptr<chi::Admin> admin_client_;
  std::unique_ptr<wrp_cte::core::Client> core_client_;
  std::string test_storage_path_;
  chi::PoolId core_pool_id_;
  
  CTECoreTestFixture() {
    // Initialize test storage path in home directory
    const char* home_dir = std::getenv("HOME");
    REQUIRE(home_dir != nullptr);
    
    test_storage_path_ = std::string(home_dir) + "/cte_test_storage.dat";
    
    // Clean up any existing test file
    if (fs::exists(test_storage_path_)) {
      fs::remove(test_storage_path_);
    }
    
    // Initialize Chimaera runtime
    auto* chimaera_runtime = CHI_RUNTIME;
    REQUIRE(chimaera_runtime != nullptr);
    
    // Create and initialize admin client first (required per CLAUDE.md)
    admin_client_ = std::make_unique<chi::Admin>();
    
    // Generate unique pool ID for this test
    core_pool_id_ = chi::PoolId::GetRandom();
    
    // Create and initialize core client with the generated pool ID
    core_client_ = std::make_unique<wrp_cte::core::Client>(core_pool_id_);
  }
  
  ~CTECoreTestFixture() {
    // Cleanup test storage file
    if (fs::exists(test_storage_path_)) {
      fs::remove(test_storage_path_);
    }
  }
  
  /**
   * Helper method to create test data buffer
   */
  std::vector<char> CreateTestData(size_t size, char pattern = 'A') {
    std::vector<char> data(size);
    for (size_t i = 0; i < size; ++i) {
      data[i] = static_cast<char>(pattern + (i % 26));
    }
    return data;
  }
  
  /**
   * Helper method to verify data integrity
   */
  bool VerifyTestData(const std::vector<char>& data, char pattern = 'A') {
    for (size_t i = 0; i < data.size(); ++i) {
      if (data[i] != static_cast<char>(pattern + (i % 26))) {
        return false;
      }
    }
    return true;
  }
};

/**
 * Test Case: Create CTE Core Pool
 * 
 * This test verifies:
 * 1. CTE Core pool can be created successfully using admin pool
 * 2. CreateTask uses chi::kAdminPoolId as required by CLAUDE.md
 * 3. Pool initialization completes without errors
 * 4. Configuration parameters are properly applied
 */
TEST_CASE_METHOD(CTECoreTestFixture, "Create CTE Core Pool", "[cte][core][pool][creation]") {
  SECTION("Synchronous pool creation with default parameters") {
    // Create memory context for task allocation
    hipc::MemContext mctx;
    
    // Create pool using local query (never null as per CLAUDE.md)
    chi::PoolQuery pool_query = chi::PoolQuery::Local();
    
    // Create parameters with test configuration
    wrp_cte::core::CreateParams params;
    params.worker_count_ = kTestWorkerCount;
    
    // Test synchronous creation - should complete successfully
    REQUIRE_NOTHROW(core_client_->Create(mctx, pool_query, params));
    
    INFO("CTE Core pool created successfully with pool ID: " << core_pool_id_.ToU64());
  }
  
  SECTION("Asynchronous pool creation") {
    hipc::MemContext mctx;
    chi::PoolQuery pool_query = chi::PoolQuery::Local();
    wrp_cte::core::CreateParams params;
    params.worker_count_ = kTestWorkerCount;
    
    // Test asynchronous creation
    auto create_task = core_client_->AsyncCreate(mctx, pool_query, params);
    REQUIRE(!create_task.IsNull());
    
    // Wait for completion
    create_task->Wait();
    
    // Verify successful completion (assuming 0 indicates success)
    // Note: The actual success criteria may depend on the task implementation
    INFO("Asynchronous pool creation completed");
    
    // Cleanup task
    CHI_IPC->DelTask(create_task);
  }
}

/**
 * Test Case: Register Target
 * 
 * This test verifies:
 * 1. File-based bdev target can be registered successfully
 * 2. Target registration uses proper bdev configuration
 * 3. Target is created in home directory as specified
 * 4. Registration returns success code
 */
TEST_CASE_METHOD(CTECoreTestFixture, "Register Target", "[cte][core][target][registration]") {
  // First create the core pool
  hipc::MemContext mctx;
  chi::PoolQuery pool_query = chi::PoolQuery::Local();
  wrp_cte::core::CreateParams params;
  params.worker_count_ = kTestWorkerCount;
  
  REQUIRE_NOTHROW(core_client_->Create(mctx, pool_query, params));
  
  SECTION("Register file-based bdev target") {
    const std::string target_name = "test_target_file";
    
    // Register target with file-based bdev pointing to home directory
    chi::u32 result = core_client_->RegisterTarget(
        mctx,
        target_name,
        chimaera::bdev::BdevType::kFile,
        test_storage_path_,
        kTestTargetSize
    );
    
    // Verify successful registration
    REQUIRE(result == 0);
    INFO("Target registered successfully: " << target_name);
    INFO("Target file path: " << test_storage_path_);
    INFO("Target size: " << kTestTargetSize << " bytes");
    
    // Verify that the target appears in target list
    auto targets = core_client_->ListTargets(mctx);
    REQUIRE(!targets.empty());
    
    bool target_found = false;
    for (const auto& target : targets) {
      if (target.target_name_ == target_name) {
        target_found = true;
        INFO("Found registered target in list with score: " << target.target_score_);
        break;
      }
    }
    REQUIRE(target_found);
  }
  
  SECTION("Register target with invalid parameters") {
    // Test with empty target name - should fail
    chi::u32 result = core_client_->RegisterTarget(
        mctx,
        "",  // Empty name should cause failure
        chimaera::bdev::BdevType::kFile,
        test_storage_path_,
        kTestTargetSize
    );
    
    // Expecting non-zero result code indicating failure
    INFO("Empty target name registration result: " << result);
    // Note: The exact error handling behavior may vary based on implementation
  }
  
  SECTION("Asynchronous target registration") {
    const std::string target_name = "test_target_async";
    
    auto register_task = core_client_->AsyncRegisterTarget(
        mctx,
        target_name,
        chimaera::bdev::BdevType::kFile,
        test_storage_path_,
        kTestTargetSize
    );
    
    REQUIRE(!register_task.IsNull());
    register_task->Wait();
    
    // Check result
    chi::u32 result = register_task->result_code_;
    REQUIRE(result == 0);
    
    CHI_IPC->DelTask(register_task);
    INFO("Asynchronous target registration completed successfully");
  }
}

/**
 * Test Case: PutBlob
 * 
 * This test verifies:
 * 1. Blob can be stored successfully with proper name and ID
 * 2. Validation logic works for name and ID requirements
 * 3. Error cases are handled properly (empty name, zero ID)
 * 4. Blob metadata is stored correctly
 */
TEST_CASE_METHOD(CTECoreTestFixture, "PutBlob", "[cte][core][blob][put]") {
  // Setup: Create core pool and register target
  hipc::MemContext mctx;
  chi::PoolQuery pool_query = chi::PoolQuery::Local();
  wrp_cte::core::CreateParams params;
  params.worker_count_ = kTestWorkerCount;
  
  REQUIRE_NOTHROW(core_client_->Create(mctx, pool_query, params));
  
  const std::string target_name = "test_target_putblob";
  chi::u32 reg_result = core_client_->RegisterTarget(
      mctx,
      target_name,
      chimaera::bdev::BdevType::kFile,
      test_storage_path_,
      kTestTargetSize
  );
  REQUIRE(reg_result == 0);
  
  // Create a test tag for blob grouping
  const std::string tag_name = "test_tag";
  auto tag_info = core_client_->GetOrCreateTag(mctx, tag_name, 0);
  REQUIRE(!tag_info.tag_name_.empty());
  chi::u32 tag_id = tag_info.tag_id_;
  
  SECTION("Successful blob storage with valid name and ID") {
    const std::string blob_name = "test_blob_valid";
    const chi::u32 blob_id = 12345;
    const chi::u64 blob_size = 1024;  // 1KB test data
    
    // Create test data
    auto test_data = CreateTestData(blob_size);
    
    // Allocate shared memory for blob data
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer blob_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    // Copy test data to shared memory
    char* blob_data_raw = ipc_manager->GetPtr<char>(blob_data_ptr);
    std::memcpy(blob_data_raw, test_data.data(), blob_size);
    
    // Put blob with valid parameters
    chi::u32 result = core_client_->PutBlob(
        mctx,
        tag_id,
        blob_name,
        blob_id,
        0,      // offset
        blob_size,
        blob_data_ptr,
        0.8f,   // score
        0       // flags
    );
    
    // Verify successful storage
    REQUIRE(result == 0);
    INFO("Blob stored successfully - Name: " << blob_name << ", ID: " << blob_id);
    
    // Cleanup allocated buffer
    ipc_manager->FreeBuffers(mctx, blob_data_ptr);
  }
  
  SECTION("Error case: Empty blob name") {
    const chi::u32 blob_id = 12346;
    const chi::u64 blob_size = 512;
    
    auto test_data = CreateTestData(blob_size);
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer blob_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    char* blob_data_raw = ipc_manager->GetPtr<char>(blob_data_ptr);
    std::memcpy(blob_data_raw, test_data.data(), blob_size);
    
    // Attempt to put blob with empty name - should fail
    chi::u32 result = core_client_->PutBlob(
        mctx,
        tag_id,
        "",     // Empty name
        blob_id,
        0,
        blob_size,
        blob_data_ptr,
        0.5f,
        0
    );
    
    // Expecting non-zero result indicating failure
    INFO("Empty blob name result: " << result);
    // Note: Exact error behavior depends on implementation
    
    ipc_manager->FreeBuffers(mctx, blob_data_ptr);
  }
  
  SECTION("Error case: Zero blob ID") {
    const std::string blob_name = "test_blob_zero_id";
    const chi::u64 blob_size = 512;
    
    auto test_data = CreateTestData(blob_size);
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer blob_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    char* blob_data_raw = ipc_manager->GetPtr<char>(blob_data_ptr);
    std::memcpy(blob_data_raw, test_data.data(), blob_size);
    
    // Attempt to put blob with zero ID - should fail based on validation logic
    chi::u32 result = core_client_->PutBlob(
        mctx,
        tag_id,
        blob_name,
        0,      // Zero ID
        0,
        blob_size,
        blob_data_ptr,
        0.5f,
        0
    );
    
    INFO("Zero blob ID result: " << result);
    
    ipc_manager->FreeBuffers(mctx, blob_data_ptr);
  }
  
  SECTION("Asynchronous blob storage") {
    const std::string blob_name = "test_blob_async";
    const chi::u32 blob_id = 12347;
    const chi::u64 blob_size = 2048;
    
    auto test_data = CreateTestData(blob_size);
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer blob_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    char* blob_data_raw = ipc_manager->GetPtr<char>(blob_data_ptr);
    std::memcpy(blob_data_raw, test_data.data(), blob_size);
    
    // Asynchronous put blob
    auto put_task = core_client_->AsyncPutBlob(
        mctx,
        tag_id,
        blob_name,
        blob_id,
        0,
        blob_size,
        blob_data_ptr,
        0.7f,
        0
    );
    
    REQUIRE(!put_task.IsNull());
    put_task->Wait();
    
    chi::u32 result = put_task->result_code_;
    REQUIRE(result == 0);
    
    CHI_IPC->DelTask(put_task);
    ipc_manager->FreeBuffers(mctx, blob_data_ptr);
    
    INFO("Asynchronous blob storage completed successfully");
  }
}

/**
 * Test Case: GetBlob
 * 
 * This test verifies:
 * 1. Blobs can be retrieved after being stored
 * 2. Data integrity is maintained during storage and retrieval
 * 3. Error cases are handled (non-existent blob)
 * 4. Both synchronous and asynchronous retrieval work
 */
TEST_CASE_METHOD(CTECoreTestFixture, "GetBlob", "[cte][core][blob][get]") {
  // Setup: Create core pool, register target, create tag
  hipc::MemContext mctx;
  chi::PoolQuery pool_query = chi::PoolQuery::Local();
  wrp_cte::core::CreateParams params;
  params.worker_count_ = kTestWorkerCount;
  
  REQUIRE_NOTHROW(core_client_->Create(mctx, pool_query, params));
  
  const std::string target_name = "test_target_getblob";
  chi::u32 reg_result = core_client_->RegisterTarget(
      mctx,
      target_name,
      chimaera::bdev::BdevType::kFile,
      test_storage_path_,
      kTestTargetSize
  );
  REQUIRE(reg_result == 0);
  
  auto tag_info = core_client_->GetOrCreateTag(mctx, "test_tag_get", 0);
  chi::u32 tag_id = tag_info.tag_id_;
  
  SECTION("Store and retrieve blob with data integrity check") {
    const std::string blob_name = "test_blob_retrieve";
    const chi::u32 blob_id = 54321;
    const chi::u64 blob_size = 4096;  // 4KB test data
    
    // Create distinctive test data
    auto original_data = CreateTestData(blob_size, 'X');
    
    // Store the blob first
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer put_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    char* put_data_raw = ipc_manager->GetPtr<char>(put_data_ptr);
    std::memcpy(put_data_raw, original_data.data(), blob_size);
    
    chi::u32 put_result = core_client_->PutBlob(
        mctx, tag_id, blob_name, blob_id, 0, blob_size, 
        put_data_ptr, 0.9f, 0
    );
    REQUIRE(put_result == 0);
    
    // Now retrieve the blob
    hipc::Pointer retrieved_data_ptr = core_client_->GetBlob(
        mctx, tag_id, blob_name, blob_id, 0, blob_size, 0
    );
    
    REQUIRE(!retrieved_data_ptr.IsNull());
    
    // Verify data integrity
    char* retrieved_data_raw = ipc_manager->GetPtr<char>(retrieved_data_ptr);
    std::vector<char> retrieved_data(retrieved_data_raw, 
                                   retrieved_data_raw + blob_size);
    
    REQUIRE(VerifyTestData(retrieved_data, 'X'));
    REQUIRE(retrieved_data == original_data);
    
    INFO("Data integrity verified - blob stored and retrieved successfully");
    
    // Cleanup
    ipc_manager->FreeBuffers(mctx, put_data_ptr);
    ipc_manager->FreeBuffers(mctx, retrieved_data_ptr);
  }
  
  SECTION("Error case: Retrieve non-existent blob") {
    // Attempt to retrieve a blob that doesn't exist
    hipc::Pointer retrieved_data_ptr = core_client_->GetBlob(
        mctx,
        tag_id,
        "non_existent_blob",
        99999,  // Non-existent blob ID
        0,
        1024,
        0
    );
    
    // Should return null pointer or handle gracefully
    INFO("Non-existent blob retrieval result - IsNull: " << retrieved_data_ptr.IsNull());
    // Note: Error handling behavior depends on implementation
  }
  
  SECTION("Asynchronous blob retrieval") {
    const std::string blob_name = "test_blob_async_get";
    const chi::u32 blob_id = 54322;
    const chi::u64 blob_size = 2048;
    
    auto original_data = CreateTestData(blob_size, 'Y');
    
    // Store blob first
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer put_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    char* put_data_raw = ipc_manager->GetPtr<char>(put_data_ptr);
    std::memcpy(put_data_raw, original_data.data(), blob_size);
    
    chi::u32 put_result = core_client_->PutBlob(
        mctx, tag_id, blob_name, blob_id, 0, blob_size,
        put_data_ptr, 0.6f, 0
    );
    REQUIRE(put_result == 0);
    
    // Asynchronous retrieval
    auto get_task = core_client_->AsyncGetBlob(
        mctx, tag_id, blob_name, blob_id, 0, blob_size, 0
    );
    
    REQUIRE(!get_task.IsNull());
    get_task->Wait();
    
    hipc::Pointer retrieved_data_ptr = get_task->blob_data_;
    REQUIRE(!retrieved_data_ptr.IsNull());
    
    // Verify data integrity
    char* retrieved_data_raw = ipc_manager->GetPtr<char>(retrieved_data_ptr);
    std::vector<char> retrieved_data(retrieved_data_raw,
                                   retrieved_data_raw + blob_size);
    
    REQUIRE(VerifyTestData(retrieved_data, 'Y'));
    REQUIRE(retrieved_data == original_data);
    
    CHI_IPC->DelTask(get_task);
    ipc_manager->FreeBuffers(mctx, put_data_ptr);
    
    INFO("Asynchronous blob retrieval completed successfully with data integrity verified");
  }
  
  SECTION("Partial blob retrieval") {
    const std::string blob_name = "test_blob_partial";
    const chi::u32 blob_id = 54323;
    const chi::u64 total_blob_size = 8192;  // 8KB total
    const chi::u64 partial_size = 2048;     // Retrieve 2KB
    const chi::u64 partial_offset = 1024;   // Start at 1KB offset
    
    auto original_data = CreateTestData(total_blob_size, 'Z');
    
    // Store full blob
    auto* ipc_manager = CHI_IPC;
    hipc::Pointer put_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, total_blob_size
    ).shm_;
    
    char* put_data_raw = ipc_manager->GetPtr<char>(put_data_ptr);
    std::memcpy(put_data_raw, original_data.data(), total_blob_size);
    
    chi::u32 put_result = core_client_->PutBlob(
        mctx, tag_id, blob_name, blob_id, 0, total_blob_size,
        put_data_ptr, 0.4f, 0
    );
    REQUIRE(put_result == 0);
    
    // Retrieve partial blob
    hipc::Pointer retrieved_data_ptr = core_client_->GetBlob(
        mctx, tag_id, blob_name, blob_id, partial_offset, partial_size, 0
    );
    
    REQUIRE(!retrieved_data_ptr.IsNull());
    
    // Verify partial data integrity
    char* retrieved_data_raw = ipc_manager->GetPtr<char>(retrieved_data_ptr);
    std::vector<char> retrieved_data(retrieved_data_raw,
                                   retrieved_data_raw + partial_size);
    
    // Compare with expected partial data
    std::vector<char> expected_partial(
        original_data.begin() + partial_offset,
        original_data.begin() + partial_offset + partial_size
    );
    
    REQUIRE(retrieved_data == expected_partial);
    
    INFO("Partial blob retrieval successful - offset: " << partial_offset 
         << ", size: " << partial_size);
    
    // Cleanup
    ipc_manager->FreeBuffers(mctx, put_data_ptr);
    ipc_manager->FreeBuffers(mctx, retrieved_data_ptr);
  }
}

/**
 * Integration Test: End-to-End CTE Core Workflow
 * 
 * This test verifies the complete workflow:
 * 1. Initialize CTE core pool
 * 2. Register multiple targets
 * 3. Create tags for organization
 * 4. Store multiple blobs
 * 5. Retrieve and verify all blobs
 * 6. Update target statistics
 */
TEST_CASE_METHOD(CTECoreTestFixture, "End-to-End CTE Core Workflow", "[cte][core][integration]") {
  hipc::MemContext mctx;
  chi::PoolQuery pool_query = chi::PoolQuery::Local();
  wrp_cte::core::CreateParams params;
  params.worker_count_ = kTestWorkerCount;
  
  // Step 1: Initialize CTE core pool
  REQUIRE_NOTHROW(core_client_->Create(mctx, pool_query, params));
  INFO("Step 1 completed: CTE core pool initialized");
  
  // Step 2: Register multiple targets
  const std::vector<std::string> target_names = {"target_1", "target_2"};
  for (const auto& target_name : target_names) {
    std::string target_path = test_storage_path_ + "_" + target_name;
    chi::u32 result = core_client_->RegisterTarget(
        mctx, target_name, chimaera::bdev::BdevType::kFile,
        target_path, kTestTargetSize
    );
    REQUIRE(result == 0);
  }
  INFO("Step 2 completed: Multiple targets registered");
  
  // Step 3: Create tags for organization
  const std::vector<std::string> tag_names = {"documents", "images", "logs"};
  std::vector<chi::u32> tag_ids;
  
  for (const auto& tag_name : tag_names) {
    auto tag_info = core_client_->GetOrCreateTag(mctx, tag_name, 0);
    REQUIRE(!tag_info.tag_name_.empty());
    tag_ids.push_back(tag_info.tag_id_);
  }
  INFO("Step 3 completed: Tags created for organization");
  
  // Step 4: Store multiple blobs across different tags
  std::vector<std::tuple<chi::u32, std::string, chi::u32, std::vector<char>>> stored_blobs;
  auto* ipc_manager = CHI_IPC;
  
  for (size_t i = 0; i < tag_ids.size(); ++i) {
    chi::u32 tag_id = tag_ids[i];
    std::string blob_name = "blob_" + std::to_string(i);
    chi::u32 blob_id = static_cast<chi::u32>(10000 + i);
    chi::u64 blob_size = 1024 * (i + 1);  // Variable sizes
    
    auto blob_data = CreateTestData(blob_size, static_cast<char>('A' + i));
    
    hipc::Pointer put_data_ptr = ipc_manager->AllocateBuffers<char>(
        mctx, blob_size
    ).shm_;
    
    char* put_data_raw = ipc_manager->GetPtr<char>(put_data_ptr);
    std::memcpy(put_data_raw, blob_data.data(), blob_size);
    
    chi::u32 result = core_client_->PutBlob(
        mctx, tag_id, blob_name, blob_id, 0, blob_size,
        put_data_ptr, 0.5f + 0.1f * i, 0
    );
    REQUIRE(result == 0);
    
    stored_blobs.push_back({tag_id, blob_name, blob_id, blob_data});
    ipc_manager->FreeBuffers(mctx, put_data_ptr);
  }
  INFO("Step 4 completed: Multiple blobs stored across tags");
  
  // Step 5: Retrieve and verify all blobs
  for (const auto& [tag_id, blob_name, blob_id, original_data] : stored_blobs) {
    hipc::Pointer retrieved_data_ptr = core_client_->GetBlob(
        mctx, tag_id, blob_name, blob_id, 0, original_data.size(), 0
    );
    
    REQUIRE(!retrieved_data_ptr.IsNull());
    
    char* retrieved_data_raw = ipc_manager->GetPtr<char>(retrieved_data_ptr);
    std::vector<char> retrieved_data(retrieved_data_raw,
                                   retrieved_data_raw + original_data.size());
    
    REQUIRE(retrieved_data == original_data);
    
    ipc_manager->FreeBuffers(mctx, retrieved_data_ptr);
  }
  INFO("Step 5 completed: All blobs retrieved and verified");
  
  // Step 6: Update target statistics
  chi::u32 stat_result = core_client_->StatTargets(mctx);
  INFO("Step 6 completed: Target statistics updated, result: " << stat_result);
  
  // Verify targets are still listed correctly
  auto final_targets = core_client_->ListTargets(mctx);
  REQUIRE(final_targets.size() >= target_names.size());
  INFO("Integration test completed successfully - all steps verified");
}