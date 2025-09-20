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
 * POSIX ADAPTER UNIT TESTS
 *
 * This test suite provides unit tests for the WRP CTE POSIX adapter that
 * directly link to the adapter libraries without using LD_PRELOAD.
 *
 * Test Cases:
 * 1. Open-Write-Read-Close: Basic file I/O operations with data verification
 */

#include <catch2/catch_all.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <filesystem>

// CTE core client for initialization
#include "chimaera/core/core_client.h"
#include "chimaera/chimaera.h"

using namespace std::chrono_literals;
namespace stdfs = std::filesystem;

// Test constants
const size_t kTestFileSize = 16 * 1024 * 1024; // 16MB
const std::string kTestDir = "/tmp";
const std::string kTestFile = "/tmp/wrp_cte_posix_test.dat";

// Global initialization flags
static bool g_runtime_initialized = false;
static bool g_client_initialized = false;

/**
 * Initialize Chimaera runtime and CTE client
 * Must be called before any POSIX adapter operations
 */
bool initializeCTE() {
  // Step 1: Initialize Chimaera runtime
  if (!g_runtime_initialized) {
    INFO("Initializing Chimaera runtime...");
    bool success = chi::CHIMAERA_RUNTIME_INIT();
    if (!success) {
      FAIL("Failed to initialize Chimaera runtime");
      return false;
    }
    g_runtime_initialized = true;
    std::this_thread::sleep_for(500ms); // Allow initialization
    INFO("✓ Chimaera runtime initialized");
  }
  
  // Step 2: Initialize CTE client
  if (!g_client_initialized) {
    INFO("Initializing CTE client...");
    bool success = wrp_cte::core::WRP_CTE_INIT();
    if (!success) {
      FAIL("Failed to initialize CTE client");
      return false;
    }
    g_client_initialized = true;
    std::this_thread::sleep_for(200ms); // Allow connection
    INFO("✓ CTE client initialized");
  }
  
  return true;
}

/**
 * POSIX Adapter Test: Open-Write-Read-Close
 *
 * This test verifies basic POSIX file operations through the CTE adapter:
 * 1. Opens a file in /tmp directory
 * 2. Writes 16MB of test data to the file
 * 3. Reads 16MB from the file
 * 4. Verifies the write and read data match
 * 5. Closes the file
 * 6. Removes the file
 */
TEST_CASE("POSIX Adapter: Open-Write-Read-Close", "[posix][adapter]") {
  // Initialize Chimaera runtime and CTE client
  REQUIRE(initializeCTE());
  
  // Clean up any existing test file
  if (stdfs::exists(kTestFile)) {
    stdfs::remove(kTestFile);
  }
  
  SECTION("Basic file I/O operations") {
    // Step 1: Open file for writing
    INFO("Step 1: Opening file for writing...");
    int fd = open(kTestFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    REQUIRE(fd >= 0);
    INFO("✓ File opened successfully with fd=" << fd);
    
    // Step 2: Prepare test data
    INFO("Step 2: Preparing 16MB test data...");
    std::vector<char> write_data(kTestFileSize);
    
    // Fill with a pattern that's easy to verify
    for (size_t i = 0; i < kTestFileSize; ++i) {
      write_data[i] = static_cast<char>(i % 256);
    }
    INFO("✓ Test data prepared with repeating byte pattern");
    
    // Step 3: Write data to file
    INFO("Step 3: Writing 16MB to file...");
    ssize_t bytes_written = write(fd, write_data.data(), kTestFileSize);
    REQUIRE(bytes_written == static_cast<ssize_t>(kTestFileSize));
    INFO("✓ Successfully wrote " << bytes_written << " bytes");
    
    // Step 4: Close file after writing
    INFO("Step 4: Closing file after writing...");
    int close_result = close(fd);
    REQUIRE(close_result == 0);
    INFO("✓ File closed successfully");
    
    // Step 5: Open file for reading
    INFO("Step 5: Opening file for reading...");
    fd = open(kTestFile.c_str(), O_RDONLY);
    REQUIRE(fd >= 0);
    INFO("✓ File reopened for reading with fd=" << fd);
    
    // Step 6: Read data from file
    INFO("Step 6: Reading 16MB from file...");
    std::vector<char> read_data(kTestFileSize);
    ssize_t bytes_read = read(fd, read_data.data(), kTestFileSize);
    REQUIRE(bytes_read == static_cast<ssize_t>(kTestFileSize));
    INFO("✓ Successfully read " << bytes_read << " bytes");
    
    // Step 7: Verify data integrity
    INFO("Step 7: Verifying data integrity...");
    bool data_matches = (write_data == read_data);
    REQUIRE(data_matches);
    INFO("✓ Data verification successful - write and read data match");
    
    // Step 8: Close file after reading
    INFO("Step 8: Closing file after reading...");
    close_result = close(fd);
    REQUIRE(close_result == 0);
    INFO("✓ File closed successfully");
    
    // Step 9: Remove test file
    INFO("Step 9: Removing test file...");
    bool removed = stdfs::remove(kTestFile);
    REQUIRE(removed);
    INFO("✓ Test file removed successfully");
  }
}

/**
 * Additional test for file size verification
 */
TEST_CASE("POSIX Adapter: File Size Verification", "[posix][adapter]") {
  // Initialize Chimaera runtime and CTE client
  REQUIRE(initializeCTE());
  
  // Clean up any existing test file
  if (stdfs::exists(kTestFile)) {
    stdfs::remove(kTestFile);
  }
  
  SECTION("Verify file size after write operations") {
    // Create and write to file
    int fd = open(kTestFile.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    REQUIRE(fd >= 0);
    
    const size_t test_size = 1024; // 1KB for quick test
    std::vector<char> data(test_size, 'A');
    
    ssize_t bytes_written = write(fd, data.data(), test_size);
    REQUIRE(bytes_written == static_cast<ssize_t>(test_size));
    
    close(fd);
    
    // Verify file size using filesystem
    auto file_size = stdfs::file_size(kTestFile);
    REQUIRE(file_size == test_size);
    
    // Clean up
    stdfs::remove(kTestFile);
  }
}