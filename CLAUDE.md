## Code Style

Use the Google C++ style guide for C++.

You should store the pointer returned by the singleton GetInstance method. Avoid dereferencing GetInstance method directly using either -> or *. E.g., do not do ``hshm::Singleton<T>::GetInstance()->var_``. You should do ``auto *x = hshm::Singleton<T>::GetInstance(); x->var_;``.


NEVER use a null pool query. If you don't know, always use local.

Local QueueId should be named. NEVER use raw integers. This is the same for priorities. Please name them semantically.

## ChiMod Client Requirements

### CreateTask Pool Assignment
CreateTask operations in all ChiMod clients MUST use `chi::kAdminPoolId` instead of the client's `pool_id_`. This is because CreateTask is actually a GetOrCreatePoolTask that must be processed by the admin ChiMod to create or find the target pool.

**Correct Usage:**
```cpp
auto task = ipc_manager->NewTask<CreateTask>(
    chi::CreateTaskNode(),
    chi::kAdminPoolId,  // Always use admin pool for CreateTask
    pool_query,
    // ... other parameters
);
```

**Incorrect Usage:**
```cpp
auto task = ipc_manager->NewTask<CreateTask>(
    chi::CreateTaskNode(),
    pool_id_,  // WRONG - this bypasses admin pool processing
    pool_query,
    // ... other parameters
);
```

This applies to all ChiMod clients including bdev, MOD_NAME, and any future ChiMods.

## Workflow
Use the incremental logic builder agent when making code changes.

Use the compiler subagent for making changes to cmakes and identifying places that need to be fixed in the code.

Always verify that code continue to compiles after making changes. Avoid commenting out code to fix compilation issues.

Whenever building unit tests, make sure to use the unit testing agent.

Whenever performing filesystem queries or executing programs, use the filesystem ops script agent.

NEVER DO MOCK CODE OR STUB CODE UNLESS SPECIFICALLY STATED OTHERWISE. ALWAYS IMPLEMENT REAL, WORKING CODE.

## Build Configuration

- Always use the debug CMakePreset when compiling code in this repo.
- Always use cmake debug preset when compiling
- All compilation warnings have been resolved as of the current state

## Code Quality Standards

### Compilation Standards
- All code must compile without warnings or errors
- Use appropriate variable types to avoid sign comparison warnings (e.g., `size_t` for container sizes)
- Mark unused variables with `(void)variable_name;` to suppress warnings when the variable is intentionally unused
- Follow strict type safety to prevent implicit conversions that generate warnings

## Cleanup Commands

### Remove Temporary CMake Files
To clean all temporary CMake files produced during build:
```bash
# Remove CMake cache and configuration files
find . -name "CMakeCache.txt" -delete
find . -name "cmake_install.cmake" -delete
find . -name "CTestTestfile.cmake" -delete

# Remove generated makefiles
find . -name "Makefile" -delete
find . -name "*.make" -delete

# Remove CMake build directories and files
find . -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "_deps" -type d -exec rm -rf {} + 2>/dev/null || true

# Remove CTest and CPack files
find . -name "DartConfiguration.tcl" -delete
find . -name "CPackConfig.cmake" -delete
find . -name "CPackSourceConfig.cmake" -delete

# Remove build directories
rm -rf build/
rm -rf out/
rm -rf cmake-build-*/

# Remove CMake temporary files
find . -name "*.cmake.in" -not -path "./CMakePresets.json" -delete 2>/dev/null || true
find . -name "CMakeDoxyfile.in" -delete 2>/dev/null || true
find . -name "CMakeDoxygenDefaults.cmake" -delete 2>/dev/null || true

# Remove any .ninja_* files if using Ninja generator
find . -name ".ninja_*" -delete 2>/dev/null || true
find . -name "build.ninja" -delete 2>/dev/null || true
find . -name "rules.ninja" -delete 2>/dev/null || true

# Remove Testing directory created by CTest
find . -name "Testing" -type d -exec rm -rf {} + 2>/dev/null || true

echo "CMake cleanup completed!"
```

## Documentation

### CTE Core API Documentation
Complete API documentation and usage guide is available at: `docs/cte/cte.md`

This documentation covers:
- Installation and linking instructions
- Complete API reference with examples
- Configuration guide
- Python bindings usage
- Advanced topics and troubleshooting

### External Integration Test
A standalone external integration test is available at: `test/unit/external/`

This test demonstrates:
- How to properly link external applications to CTE Core libraries
- Complete API usage examples with error handling
- Proper initialization and cleanup patterns
- CMake configuration for external projects

To run the external test:
```bash
cd test/unit/external
mkdir -p build && cd build
cmake ..
make
./cte_external_test
```

## Development Workflow

- Never call chi_refresh_repo directly. Use bash env.sh instead.