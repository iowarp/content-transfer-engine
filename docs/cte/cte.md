# CTE Core API Documentation

## Overview

The Content Transfer Engine (CTE) Core is a high-performance distributed storage middleware system built on the Chimaera framework. It provides a flexible blob storage API with advanced features including:

- **Multi-target Storage Management**: Register and manage multiple storage backends (file, RAM, NVMe)
- **Blob Storage with Tags**: Store and retrieve data blobs with tag-based organization
- **Block-based Data Management**: Efficient block-level data placement across multiple targets
- **Performance Monitoring**: Built-in telemetry and performance metrics collection
- **Configurable Data Placement**: Multiple data placement algorithms (random, round-robin, max bandwidth)
- **Asynchronous Operations**: Both synchronous and asynchronous APIs for all operations

CTE Core implements a ChiMod (Chimaera Module) that integrates with the Chimaera distributed runtime system, providing scalable data management across multiple nodes in a cluster.

## Installation & Linking

### Prerequisites

- CMake 3.20 or higher
- C++17 compatible compiler
- Chimaera framework (chimaera-core and chimaera-admin packages)
- yaml-cpp library
- Python 3.7+ (for Python bindings)
- nanobind (for Python bindings)

### Building CTE Core

```bash
# Clone the repository
git clone <repository-url>
cd content-transfer-engine

# Create build directory
mkdir build && cd build

# Configure with CMake (using debug preset as recommended)
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the project
make -j

# Install (optional)
sudo make install
```

### Linking to CTE Core in CMake Projects

To use CTE Core in your CMake project, add the following to your `CMakeLists.txt`:

```cmake
# Find the CTE Core package
find_package(chimaera-core REQUIRED)

# Create your executable or library
add_executable(my_app main.cpp)

# Link against CTE Core client library
target_link_libraries(my_app 
  PRIVATE 
    wrp_cte_core_client
    chimaera::chimaera-core
)

# Include CTE Core headers
target_include_directories(my_app 
  PRIVATE 
    ${CMAKE_INSTALL_PREFIX}/include
)
```

### Runtime Dependencies

The CTE Core runtime library (`wrp_cte_core_runtime.so`) must be available at runtime. It will be automatically loaded by the Chimaera framework when the CTE Core container is created.

## API Reference

### Core Client Class

The main entry point for CTE Core functionality is the `wrp_cte::core::Client` class.

#### Class Definition

```cpp
namespace wrp_cte::core {

class Client : public chi::ContainerClient {
public:
  // Constructors
  Client();
  explicit Client(const chi::PoolId &pool_id);
  
  // Container lifecycle
  void Create(const hipc::MemContext &mctx, 
              const chi::PoolQuery &pool_query,
              const CreateParams &params = CreateParams());
  
  // Target management
  chi::u32 RegisterTarget(const hipc::MemContext &mctx,
                          const std::string &target_name,
                          chimaera::bdev::BdevType bdev_type,
                          chi::u64 total_size);
                          
  chi::u32 UnregisterTarget(const hipc::MemContext &mctx,
                            const std::string &target_name);
                            
  std::vector<TargetInfo> ListTargets(const hipc::MemContext &mctx);
  
  chi::u32 StatTargets(const hipc::MemContext &mctx);
  
  // Tag management  
  TagInfo GetOrCreateTag(const hipc::MemContext &mctx,
                        const std::string &tag_name,
                        const TagId &tag_id = TagId::GetNull());
                        
  bool DelTag(const hipc::MemContext &mctx, const TagId &tag_id);
  bool DelTag(const hipc::MemContext &mctx, const std::string &tag_name);
  
  size_t GetTagSize(const hipc::MemContext &mctx, const TagId &tag_id);
  
  // Blob operations
  bool PutBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id,
               chi::u64 offset, chi::u64 size, hipc::Pointer blob_data,
               float score, chi::u32 flags);
               
  bool GetBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id,
               chi::u64 offset, chi::u64 size, chi::u32 flags,
               hipc::Pointer blob_data);
               
  bool DelBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id);
               
  chi::u32 ReorganizeBlob(const hipc::MemContext &mctx, 
                          const BlobId &blob_id,
                          float new_score);
  
  // Telemetry
  std::vector<CteTelemetry> PollTelemetryLog(const hipc::MemContext &mctx,
                                             std::uint64_t minimum_logical_time);
  
  // Async variants (all methods have Async versions)
  hipc::FullPtr<CreateTask> AsyncCreate(...);
  hipc::FullPtr<RegisterTargetTask> AsyncRegisterTarget(...);
  // ... etc
};

}  // namespace wrp_cte::core
```

### Data Structures

#### CreateParams

Configuration parameters for CTE container creation:

```cpp
struct CreateParams {
  chi::string config_file_path_;  // YAML config file path
  chi::u32 worker_count_;         // Number of worker threads (default: 4)
  
  CreateParams();
  CreateParams(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc, 
               const std::string& config_file_path = "", 
               chi::u32 worker_count = 4);
};
```

#### TargetInfo

Information about a registered storage target:

```cpp
struct TargetInfo {
  std::string target_name_;
  std::string bdev_pool_name_;
  chimaera::bdev::Client bdev_client_;
  chi::u64 bytes_read_;
  chi::u64 bytes_written_;
  chi::u64 ops_read_;
  chi::u64 ops_written_;
  float target_score_;              // 0-1, normalized log bandwidth
  chi::u64 remaining_space_;        // Remaining allocatable space
  chimaera::bdev::PerfMetrics perf_metrics_;
};
```

#### TagInfo

Tag information for blob grouping:

```cpp
struct TagInfo {
  std::string tag_name_;
  TagId tag_id_;
  std::unordered_map<BlobId, chi::u32> blob_ids_;
  size_t total_size_;
  Timestamp last_modified_;
  Timestamp last_read_;
};
```

#### BlobInfo

Blob metadata and block management:

```cpp
struct BlobInfo {
  BlobId blob_id_;
  std::string blob_name_;
  std::vector<BlobBlock> blocks_;  // Ordered blocks making up the blob
  float score_;                    // 0-1 score for reorganization
  Timestamp last_modified_;
  Timestamp last_read_;
  
  chi::u64 GetTotalSize() const;   // Total size from all blocks
};
```

#### BlobBlock

Individual block within a blob:

```cpp
struct BlobBlock {
  chimaera::bdev::Client bdev_client_;  // Target client for this block
  chi::u64 target_offset_;             // Offset within target
  chi::u64 size_;                      // Size of this block
};
```

#### CteTelemetry

Telemetry data for performance monitoring:

```cpp
struct CteTelemetry {
  CteOp op_;                    // Operation type
  size_t off_;                  // Offset within blob
  size_t size_;                 // Size of operation
  BlobId blob_id_;
  TagId tag_id_;
  Timestamp mod_time_;
  Timestamp read_time_;
  std::uint64_t logical_time_;  // For ordering entries
};

enum class CteOp : chi::u32 {
  kPutBlob = 0,
  kGetBlob = 1,
  kDelBlob = 2,
  kGetOrCreateTag = 3,
  kDelTag = 4,
  kGetTagSize = 5
};
```

### Global Access

CTE Core provides singleton access patterns:

```cpp
// Initialize CTE client (must be called once)
bool WRP_CTE_CLIENT_INIT(const std::string &config_path = "");

// Access global CTE client instance
auto *client = WRP_CTE_CLIENT;

// Access global CTE configuration
auto *config = WRP_CTE_CONFIG;
```

## Usage Examples

### Basic Initialization

```cpp
#include <chimaera/chimaera.h>
#include <chimaera/core/core_client.h>
#include <chimaera/core/core_tasks.h>

int main() {
  // Initialize Chimaera runtime
  chi::CHIMAERA_RUNTIME_INIT();
  
  // Initialize Chimaera client
  chi::CHIMAERA_CLIENT_INIT();
  
  // Initialize CTE subsystem
  wrp_cte::core::WRP_CTE_CLIENT_INIT("/path/to/config.yaml");
  
  // Create CTE client
  wrp_cte::core::Client cte_client;
  
  // Create CTE container with custom parameters
  hipc::MemContext mctx;
  wrp_cte::core::CreateParams params;
  params.worker_count_ = 8;
  params.config_file_path_ = "/path/to/cte_config.yaml";
  
  cte_client.Create(mctx, chi::PoolQuery::Local(), params);
  
  return 0;
}
```

### Registering Storage Targets

```cpp
// Register a file-based storage target
std::string target_path = "/mnt/nvme/cte_storage.dat";
chi::u64 target_size = 100ULL * 1024 * 1024 * 1024;  // 100GB

chi::u32 result = cte_client.RegisterTarget(
    mctx,
    target_path,
    chimaera::bdev::BdevType::kFile,
    target_size
);

if (result == 0) {
    std::cout << "Target registered successfully\n";
}

// Register a RAM-based cache target
result = cte_client.RegisterTarget(
    mctx,
    "/tmp/cte_cache",
    chimaera::bdev::BdevType::kRam,
    8ULL * 1024 * 1024 * 1024  // 8GB
);

// List all registered targets
auto targets = cte_client.ListTargets(mctx);
for (const auto& target : targets) {
    std::cout << "Target: " << target.target_name_ << "\n"
              << "  Remaining space: " << target.remaining_space_ << " bytes\n"
              << "  Score: " << target.target_score_ << "\n";
}
```

### Working with Tags and Blobs

```cpp
// Create or get a tag for grouping related blobs
auto tag_info = cte_client.GetOrCreateTag(mctx, "dataset_v1");
TagId tag_id = tag_info.tag_id_;

// Prepare data for storage
std::vector<char> data(1024 * 1024);  // 1MB of data
std::fill(data.begin(), data.end(), 'A');

// Store blob with automatic ID generation
hipc::Pointer data_ptr = hipc::Pointer::GetNull();  // Convert data to shared memory
BlobId blob_id = BlobId::GetNull();  // Will be auto-generated

bool success = cte_client.PutBlob(
    mctx,
    tag_id,
    "blob_001",           // Blob name
    blob_id,              // Auto-generate ID
    0,                    // Offset
    data.size(),          // Size
    data_ptr,             // Data pointer
    0.8f,                 // Score (0-1, higher = hotter data)
    0                     // Flags
);

if (success) {
    std::cout << "Blob stored successfully\n";
}

// Retrieve the blob
std::vector<char> retrieved_data(1024 * 1024);
hipc::Pointer retrieve_ptr = hipc::Pointer::GetNull();

success = cte_client.GetBlob(
    mctx,
    tag_id,
    "blob_001",           // Blob name for lookup
    BlobId::GetNull(),    // Or use blob_id if known
    0,                    // Offset
    data.size(),          // Size to read
    0,                    // Flags
    retrieve_ptr          // Buffer for data
);

// Get total size of all blobs in tag
size_t tag_size = cte_client.GetTagSize(mctx, tag_id);
std::cout << "Tag total size: " << tag_size << " bytes\n";

// Delete a specific blob
success = cte_client.DelBlob(mctx, tag_id, "blob_001", blob_id);

// Delete entire tag (removes all blobs)
success = cte_client.DelTag(mctx, tag_id);
```

### Asynchronous Operations

```cpp
// Asynchronous blob operations for better performance
auto put_task = cte_client.AsyncPutBlob(
    mctx, tag_id, "async_blob", BlobId::GetNull(),
    0, data.size(), data_ptr, 0.5f, 0
);

// Do other work while blob is being stored
ProcessOtherData();

// Wait for completion
put_task->Wait();
if (put_task->result_code_ == 0) {
    std::cout << "Async put completed successfully\n";
}

// Clean up task
CHI_IPC->DelTask(put_task);

// Multiple async operations
std::vector<hipc::FullPtr<PutBlobTask>> tasks;
for (int i = 0; i < 10; ++i) {
    auto task = cte_client.AsyncPutBlob(
        mctx, tag_id, 
        "blob_" + std::to_string(i),
        BlobId::GetNull(),
        0, data.size(), data_ptr, 0.5f, 0
    );
    tasks.push_back(task);
}

// Wait for all to complete
for (auto& task : tasks) {
    task->Wait();
    CHI_IPC->DelTask(task);
}
```

### Performance Monitoring

```cpp
// Poll telemetry log for performance analysis
std::uint64_t last_logical_time = 0;

auto telemetry = cte_client.PollTelemetryLog(mctx, last_logical_time);

for (const auto& entry : telemetry) {
    std::cout << "Operation: ";
    switch (entry.op_) {
        case CteOp::kPutBlob: std::cout << "PUT"; break;
        case CteOp::kGetBlob: std::cout << "GET"; break;
        case CteOp::kDelBlob: std::cout << "DEL"; break;
        default: std::cout << "OTHER"; break;
    }
    std::cout << " Size: " << entry.size_ 
              << " Offset: " << entry.off_
              << " LogicalTime: " << entry.logical_time_ << "\n";
}

// Update target statistics
cte_client.StatTargets(mctx);

// Check updated target metrics
auto targets = cte_client.ListTargets(mctx);
for (const auto& target : targets) {
    std::cout << "Target: " << target.target_name_ << "\n"
              << "  Bytes read: " << target.bytes_read_ << "\n"
              << "  Bytes written: " << target.bytes_written_ << "\n"
              << "  Read ops: " << target.ops_read_ << "\n"
              << "  Write ops: " << target.ops_written_ << "\n";
}
```

### Blob Reorganization

```cpp
// Reorganize blob based on new access patterns
// Higher scores (closer to 1.0) indicate hotter data
float new_score = 0.95f;  // Very hot data

chi::u32 result = cte_client.ReorganizeBlob(mctx, blob_id, new_score);

if (result == 0) {
    std::cout << "Blob reorganized successfully\n";
}
```

## Configuration

CTE Core uses YAML configuration files for runtime parameters. Configuration can be loaded from:
1. A file path specified during initialization
2. Environment variable `WRP_CTE_CONF`
3. Programmatically via the Config API

### Configuration File Format

```yaml
# Worker thread configuration
worker_count: 4

# Target management settings
targets:
  max_targets: 100
  default_target_timeout_ms: 30000
  auto_unregister_failed: true

# Performance tuning
performance:
  target_stat_interval_ms: 5000      # Target statistics update interval
  blob_cache_size_mb: 512            # Cache size for blob operations
  max_concurrent_operations: 64      # Max concurrent I/O operations
  score_threshold: 0.7               # Threshold for blob reorganization

# Queue configuration for different operation types
queues:
  target_management:
    lane_count: 2
    priority: "low_latency"
  
  tag_management:
    lane_count: 2
    priority: "low_latency"
  
  blob_operations:
    lane_count: 4
    priority: "high_latency"
  
  stats:
    lane_count: 1
    priority: "high_latency"

# Storage device configuration
storage:
  # Primary high-performance storage
  - path: "/mnt/nvme/cte_primary"
    bdev_type: "file"
    capacity_limit: "1TB"
  
  # RAM-based cache
  - path: "/tmp/cte_cache"
    bdev_type: "ram"
    capacity_limit: "8GB"
  
  # Secondary storage
  - path: "/mnt/ssd/cte_secondary"
    bdev_type: "file"
    capacity_limit: "500GB"

# Data Placement Engine configuration
dpe:
  dpe_type: "max_bw"  # Options: "random", "round_robin", "max_bw"
```

### Programmatic Configuration

```cpp
#include <chimaera/core/core_config.h>

// Access configuration manager
auto& config_mgr = wrp_cte::core::ConfigManager::GetInstance();

// Load configuration from file
bool success = config_mgr.LoadConfig("/path/to/config.yaml");

// Or load from environment
success = config_mgr.LoadConfigFromEnvironment();

// Access configuration values
const auto& config = config_mgr.GetConfig();
std::cout << "Worker count: " << config.worker_count_ << "\n";
std::cout << "Max targets: " << config.targets_.max_targets_ << "\n";

// Modify configuration at runtime
auto& mutable_config = config_mgr.GetMutableConfig();
mutable_config.performance_.max_concurrent_operations_ = 128;
mutable_config.performance_.score_threshold_ = 0.85f;

// Save modified configuration
config.SaveToFile("/path/to/new_config.yaml");
```

### Queue Priority Options

- `"low_latency"` - Optimized for minimal latency (chi::kLowLatency)
- `"high_latency"` - Optimized for throughput (chi::kHighLatency)

### Storage Device Types

- `"file"` - File-based block device
- `"ram"` - RAM-based block device (for caching)
- `"dev_dax"` - Persistent memory device
- `"posix"` - POSIX file system interface

### Data Placement Engine Types

- `"random"` - Random placement across targets
- `"round_robin"` - Round-robin placement
- `"max_bw"` - Place on target with maximum available bandwidth

## Python Bindings

CTE Core provides Python bindings for easy integration with Python applications.

### Installation

```bash
# Build Python bindings
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
make

# Install Python module
pip install ./wrapper/python
```

### Python API Usage

```python
import wrp_cte_core_ext as cte

# Initialize Chimaera runtime
cte.chimaera_runtime_init()
cte.chimaera_client_init()

# Initialize CTE
cte.initialize_cte("/path/to/config.yaml")

# Get global CTE client
client = cte.get_cte_client()

# Create memory context
mctx = cte.MemContext()

# Poll telemetry log
minimum_logical_time = 0
telemetry_entries = client.PollTelemetryLog(mctx, minimum_logical_time)

for entry in telemetry_entries:
    print(f"Operation: {entry.op_}")
    print(f"Size: {entry.size_}")
    print(f"Offset: {entry.off_}")
    print(f"Logical Time: {entry.logical_time_}")
```

### Python Data Types

```python
# Create unique IDs
tag_id = cte.TagId.GetNull()
blob_id = cte.BlobId.GetNull()

# Check if ID is null
if tag_id.IsNull():
    print("Tag ID is null")

# Access ID components
print(f"Major: {tag_id.major_}, Minor: {tag_id.minor_}")

# Operation types
print(cte.CteOp.kPutBlob)    # Put blob operation
print(cte.CteOp.kGetBlob)    # Get blob operation
print(cte.CteOp.kDelBlob)    # Delete blob operation
```

## Advanced Topics

### Multi-Node Deployment

CTE Core supports distributed deployment across multiple nodes:

1. Configure Chimaera for multi-node operation
2. Use appropriate PoolQuery values:
   - `chi::PoolQuery::Local()` - Local node only
   - `chi::PoolQuery::Global()` - All nodes
   - Custom pool queries for specific node groups

### Custom Data Placement Algorithms

Extend the DPE (Data Placement Engine) by implementing custom placement strategies:

1. Inherit from the base DPE class
2. Implement placement logic based on target metrics
3. Register the new DPE type in configuration

### Performance Optimization

1. **Batch Operations**: Use async APIs for multiple operations
2. **Score-based Placement**: Set appropriate scores (0-1) for data temperature
3. **Target Balancing**: Monitor and rebalance based on target metrics
4. **Queue Tuning**: Adjust lane counts and priorities based on workload

### Error Handling

All operations return result codes:
- `0`: Success
- Non-zero: Error (specific codes depend on operation)

Always check return values and handle errors appropriately:

```cpp
chi::u32 result = cte_client.RegisterTarget(...);
if (result != 0) {
    // Handle error
    std::cerr << "Failed to register target, error code: " << result << "\n";
}
```

### Thread Safety

- CTE Core client operations are thread-safe
- Multiple threads can share a client instance
- Async operations are particularly suitable for multi-threaded usage

### Memory Management

- CTE Core uses shared memory for zero-copy data transfer
- The `hipc::Pointer` type represents shared memory locations
- Memory contexts (`hipc::MemContext`) manage allocation lifecycle

## Troubleshooting

### Common Issues

1. **Initialization Failures**
   - Ensure Chimaera runtime is initialized first
   - Check configuration file path and format
   - Verify storage paths have appropriate permissions

2. **Target Registration Errors**
   - Confirm target path exists and is writable
   - Check available disk space
   - Verify bdev type matches storage medium

3. **Blob Operations Failing**
   - Ensure tag exists before blob operations
   - Check target has sufficient space
   - Verify data pointers are valid shared memory

4. **Performance Issues**
   - Monitor target statistics regularly
   - Adjust worker count based on workload
   - Tune queue configurations
   - Consider data placement strategy

### Debug Logging

Enable debug logging by setting environment variables:

```bash
export CHIMAERA_LOG_LEVEL=DEBUG
export CTE_LOG_LEVEL=DEBUG
```

### Metrics Collection

Use the telemetry API to collect performance metrics:

```cpp
// Continuous monitoring loop
while (running) {
    auto telemetry = cte_client.PollTelemetryLog(mctx, last_logical_time);
    ProcessTelemetry(telemetry);
    
    if (!telemetry.empty()) {
        last_logical_time = telemetry.back().logical_time_;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

## API Stability and Versioning

CTE Core follows semantic versioning:
- Major version: Breaking API changes
- Minor version: New features, backward compatible
- Patch version: Bug fixes

Check version compatibility:

```cpp
// Version macros (defined in headers)
#if CTE_CORE_VERSION_MAJOR >= 1 && CTE_CORE_VERSION_MINOR >= 0
    // Use newer API features
#endif
```

## Support and Resources

- **Documentation**: This document and inline API documentation
- **Examples**: See `test/unit/` directory for comprehensive examples
- **Configuration**: Example configs in `config/` directory
- **Issues**: Report bugs via project issue tracker