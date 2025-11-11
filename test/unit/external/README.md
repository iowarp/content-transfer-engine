# External CTE Core Integration Test

This directory contains a standalone test that demonstrates how external applications can link to and use the CTE Core library. The test is **intentionally separate** from the main build system to show real-world integration patterns.

## Purpose

This test serves multiple purposes:

1. **Validates External Linking**: Ensures that the CTE Core libraries can be properly linked by external applications
2. **API Usage Example**: Demonstrates proper usage patterns for the CTE Core API
3. **Integration Verification**: Tests that all dependencies are correctly resolved when using CTE Core externally
4. **Documentation**: Provides working code examples for developers integrating CTE Core

## Building and Running

### Prerequisites

1. **Build CTE Core First**: The main CTE Core libraries must be built before running this test:
   ```bash
   # From the repository root
   mkdir -p build && cd build
   cmake ..
   make -j4
   ```

2. **Dependencies**: Ensure all CTE Core dependencies are available:
   - Chimaera framework (chimaera-core, chimaera-admin)
   - HSHM (Hermes Shared Memory)
   - yaml-cpp

### Build the External Test

```bash
# Navigate to the external test directory
cd test/unit/external

# Create build directory
mkdir -p build && cd build

# Configure with CMake
cmake ..

# Build the test
make

# Run the test
./cte_external_test
```

### Alternative: Use the run target

```bash
# From the build directory
make run_external_test
```

## What the Test Does

The test performs a comprehensive validation of CTE Core functionality:

### 1. Initialization Sequence
- Initializes Chimaera runtime and client
- Initializes CTE subsystem
- Creates CTE container with configuration

### 2. Storage Target Management
- Registers a file-based storage target
- Lists available storage targets
- Verifies target registration

### 3. Data Operations
- Creates tags for organizing data
- Stores blob data using `PutBlob`
- Retrieves blob data using `GetBlob`
- Verifies data integrity

### 4. Monitoring and Telemetry
- Collects performance telemetry data
- Displays telemetry entries
- Verifies telemetry collection functionality

### 5. Size and Metadata Operations
- Gets tag sizes
- Verifies size calculations
- Tests metadata consistency

### 6. Cleanup Operations
- Deletes blobs and tags
- Cleans up resources
- Verifies proper cleanup

## Expected Output

When successful, the test produces output like:

```
=== External CTE Core Integration Test ===
Initializing CTE Core system...
1. Initializing Chimaera runtime...
2. Initializing Chimaera client...
3. Initializing CTE subsystem...
4. Getting CTE client instance...
5. Creating CTE container...
   CTE container created successfully
CTE Core initialization completed successfully!

=== Running CTE Core API Tests ===

--- Test 1: Register Storage Target ---
✅ Storage target registered successfully

--- Test 2: Create Tag and Store Blob ---
✅ Tag created/retrieved: external_test_tag
✅ Blob stored successfully

--- Test 3: Retrieve Blob Data ---
✅ Blob retrieved and data verified successfully

--- Test 4: Test Telemetry Collection ---
✅ Retrieved 3 telemetry entries
   Entry 0: op=3, size=0, logical_time=1
   Entry 1: op=0, size=1024, logical_time=2
   Entry 2: op=1, size=1024, logical_time=3

--- Test 5: List Storage Targets ---
✅ Found 1 registered targets
   Target 0: /tmp/cte_external_test_target (score: 0.5)

--- Test 6: Get Tag Size ---
✅ Tag size: 1024 bytes
✅ Tag size verification passed

--- Test 7: Cleanup Operations ---
✅ Blob deleted successfully
✅ Tag deleted successfully

=== Test Results ===
🎉 External CTE Core integration test PASSED!
The CTE Core library is properly linkable and functional.
```

## CMake Integration Pattern

The `CMakeLists.txt` in this directory demonstrates the **proper MODULE_DEVELOPMENT_GUIDE.md patterns** for external applications to link with CTE Core ChiMods:

### Key Patterns (Updated to Follow Guide):

1. **Package Discovery** (Modern Pattern):
   ```cmake
   # Find required Chimaera framework packages
   find_package(chimaera REQUIRED)              # Core library
   find_package(chimaera_admin REQUIRED)        # Admin ChiMod
   
   # Find CTE Core ChiMod package
   find_package(wrp_cte_core REQUIRED)          # CTE Core ChiMod
   ```

2. **Library Linking** (Modern Target Names):
   ```cmake
   target_link_libraries(your_app
       # CTE Core ChiMod libraries (recommended aliases)
       wrp_cte::core_client                     # CTE Core client
       wrp_cte::core_runtime                    # CTE Core runtime (optional)
       
       # Framework dependencies automatically included
       # chimaera::cxx                          # NOT needed - auto-included
       # chimaera::admin_client                 # Optional - if needed
   )
   ```

3. **Target Naming System**:
   - **Package Names**: `wrp_cte_core` (for `find_package()`)
   - **Target Aliases**: `wrp_cte::core_client`, `wrp_cte::core_runtime` (recommended)
   - **Actual Targets**: `wrp_cte_core_client`, `wrp_cte_core_runtime`

4. **Automatic Dependencies**:
   - ChiMod targets automatically include `chimaera::cxx` framework
   - No need to manually link core framework libraries
   - `add_chimod_both()` handles all standard dependencies

## Troubleshooting

### Common Issues:

1. **Library Not Found**: Ensure CTE Core is built and libraries exist in the build directory
2. **Missing Dependencies**: Verify all dependencies (Chimaera, HSHM, etc.) are properly installed
3. **Runtime Initialization Failure**: Check that the runtime environment is properly configured
4. **Permission Issues**: Ensure write permissions for temporary files (e.g., `/tmp/cte_external_test_target`)

### Debug Mode:

Build in debug mode for more detailed error information:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Environment Variables:

Some dependencies may require environment variables:
```bash
export PKG_CONFIG_PATH=/path/to/chimaera/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/path/to/chimaera/lib:$LD_LIBRARY_PATH
```

## Integration Notes

This test demonstrates several important patterns for external CTE Core integration:

1. **Proper Initialization Order**: Chimaera runtime → Chimaera client → CTE subsystem
2. **Memory Management**: Using HSHM allocators for shared data
3. **Error Handling**: Checking return codes and handling exceptions
4. **Resource Cleanup**: Proper cleanup of tags, blobs, and resources
5. **API Usage**: Correct parameter passing and result handling

External applications should follow these same patterns for reliable CTE Core integration.