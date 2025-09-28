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

### ChiMod Build Patterns

This project follows the Chimaera MODULE_DEVELOPMENT_GUIDE.md patterns for proper ChiMod development:

**Required Packages for ChiMod Development:**
```cmake
# Core Chimaera framework (includes ChimaeraCommon.cmake functions)
find_package(chimaera REQUIRED)              # Core library (chimaera::cxx)
find_package(chimaera_admin REQUIRED)        # Admin ChiMod (required for most ChiMods)
```

**ChiMod Creation Pattern:**
```cmake
# Use modern ChiMod build functions instead of manual add_library
add_chimod_runtime(
  CHIMOD_NAME core
  SOURCES 
    src/core_runtime.cc
    src/core_config.cc
    src/autogen/core_lib_exec.cc
)

add_chimod_client(
  CHIMOD_NAME core
  SOURCES 
    src/core_client.cc
    src/content_transfer_engine.cc
)
```

**Target Naming:**
- **Actual Targets**: `${NAMESPACE}_${CHIMOD_NAME}_runtime`, `${NAMESPACE}_${CHIMOD_NAME}_client`
- **CMake Aliases**: `${NAMESPACE}::${CHIMOD_NAME}_runtime`, `${NAMESPACE}::${CHIMOD_NAME}_client` (recommended)
- **Package Names**: `${NAMESPACE}_${CHIMOD_NAME}` (for external find_package)

### Compilation Standards
- Always use the debug CMakePreset when compiling code in this repo
- Never hardcode paths in CMakeLists.txt files
- Use find_package() for all dependencies
- Follow ChiMod build patterns from MODULE_DEVELOPMENT_GUIDE.md
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

This test demonstrates **MODULE_DEVELOPMENT_GUIDE.md compliant patterns**:
- Modern find_package() usage for ChiMod discovery
- Proper target linking with namespace::module_type aliases
- Automatic dependency resolution through ChiMod targets
- External application CMake configuration

**Modern External Linking Pattern:**
```cmake
# Find required packages
find_package(chimaera REQUIRED)              # Core framework
find_package(chimaera_admin REQUIRED)        # Admin ChiMod
find_package(wrp_cte_core REQUIRED)          # CTE Core ChiMod

# Link using modern target aliases (recommended)
target_link_libraries(my_app
    wrp_cte::core_client                      # CTE client library
    # Framework dependencies auto-included by ChiMod targets
)
```

To run the external test:
```bash
cd test/unit/external
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
./cte_external_test
```

## Development Workflow

### ChiMod Development

**Build System Requirements:**
1. **Never hardcode paths** - always use find_package() and proper CMake variables
2. **Use ChiMod build functions** - `add_chimod_runtime()`, `add_chimod_client()`, or `add_chimod_both()`
3. **Follow target naming** - use namespace::module_type patterns
4. **Automatic dependencies** - ChiMod functions handle core dependencies automatically

**Required find_package patterns:**
```cmake
# For internal ChiMod development:
find_package(chimaera REQUIRED)              # Core framework
find_package(chimaera_admin REQUIRED)        # Admin ChiMod

# For external applications:
find_package(chimaera REQUIRED)              # Core framework  
find_package(wrp_cte_core REQUIRED)          # CTE ChiMod package
```

**Build Commands:**
- Never call chi_refresh_repo directly. Use bash env.sh instead
- Always use debug cmake preset: `cmake --preset=debug`
- Build: `cmake --build build --config Debug`