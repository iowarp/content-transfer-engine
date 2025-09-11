#ifndef BDEV_TASKS_H_
#define BDEV_TASKS_H_

#include <chimaera/chimaera.h>
#include <chimaera/bdev/autogen/bdev_methods.h>
// Include admin tasks for GetOrCreatePoolTask
#include <chimaera/admin/admin_tasks.h>

namespace chimaera::bdev_extended {

/**
 * Backend type for bdev storage
 */
enum class BdevType : chi::u32 {
  kFile = 0,  // File-based block device (default)
  kRam = 1    // RAM-based block device
};

/**
 * Block structure representing an allocated block of storage
 */
struct Block {
  chi::u64 offset_;     // Offset within file/device
  chi::u64 size_;       // Size of block in bytes
  chi::u32 block_type_; // Block size category (0=4KB, 1=64KB, 2=256KB, 3=1MB)
  
  Block() : offset_(0), size_(0), block_type_(0) {}
  Block(chi::u64 offset, chi::u64 size, chi::u32 block_type)
    : offset_(offset), size_(size), block_type_(block_type) {}
};

/**
 * Performance metrics structure
 */
struct PerfMetrics {
  double read_bandwidth_mbps_;   // Read bandwidth in MB/s
  double write_bandwidth_mbps_;  // Write bandwidth in MB/s
  double read_latency_us_;       // Average read latency in microseconds
  double write_latency_us_;      // Average write latency in microseconds
  double iops_;                  // I/O operations per second
  
  PerfMetrics() : read_bandwidth_mbps_(0.0), write_bandwidth_mbps_(0.0),
                  read_latency_us_(0.0), write_latency_us_(0.0), iops_(0.0) {}
};

/**
 * CreateParams for bdev chimod
 * Contains configuration parameters for bdev container creation
 */
struct CreateParams {
  BdevType bdev_type_;         // Block device type (file or RAM)
  std::string file_path_;      // Path to block device file (for kFile type)
  chi::u64 total_size_;        // Total size for allocation (0 = file size for kFile, required for kRam)
  chi::u32 io_depth_;          // libaio queue depth (ignored for kRam, default: 32)
  chi::u32 alignment_;         // I/O alignment in bytes (default: 4096)
  
  // Required: chimod library name for module manager
  static constexpr const char* chimod_lib_name = "wrp_cte_bdev_extended";
  
  // Default constructor
  CreateParams() : bdev_type_(BdevType::kFile), total_size_(0), 
                   io_depth_(32), alignment_(4096) {}
  
  // Constructor with allocator and parameters
  CreateParams(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc, 
               BdevType bdev_type = BdevType::kFile,
               const std::string& file_path = "", 
               chi::u64 total_size = 0,
               chi::u32 io_depth = 32,
               chi::u32 alignment = 4096)
      : bdev_type_(bdev_type), file_path_(file_path), total_size_(total_size),
        io_depth_(io_depth), alignment_(alignment) {
    // Bdev parameters use standard types, so allocator isn't needed directly
    // but it's available for future use with HSHM containers
  }
  
  // Serialization support for cereal
  template<class Archive>
  void serialize(Archive& ar) {
    ar(bdev_type_, file_path_, total_size_, io_depth_, alignment_);
  }
};

/**
 * CreateTask - Initialize the bdev container
 * Type alias for GetOrCreatePoolTask with CreateParams (uses kGetOrCreatePool method)
 * Non-admin modules should use GetOrCreatePoolTask instead of BaseCreateTask
 */
using CreateTask = chimaera::admin::GetOrCreatePoolTask<CreateParams>;

/**
 * Block allocation task
 */
struct AllocateTask : public chi::Task {
  // Task-specific data using HSHM macros
  IN chi::u64 size_;           // Requested block size in bytes
  OUT Block block_;            // Allocated block information (output)
  OUT chi::u32 result_code_;   // Operation result (0 = success)

  // SHM constructor
  explicit AllocateTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        size_(0), 
        block_(),
        result_code_(0) {}

  // Emplace constructor
  explicit AllocateTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      chi::u64 size)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kAllocate),
        size_(size),
        block_(),
        result_code_(0) {}
};

/**
 * Block deallocation task
 */
struct FreeTask : public chi::Task {
  IN Block block_;             // Block to free
  OUT chi::u32 result_code_;   // Operation result (0 = success)

  // SHM constructor
  explicit FreeTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        block_(),
        result_code_(0) {}

  // Emplace constructor
  explicit FreeTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const Block &block)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kFree),
        block_(block),
        result_code_(0) {}
};

/**
 * Block write operation task
 */
struct WriteTask : public chi::Task {
  IN Block block_;                    // Target block for writing
  INOUT chi::vector<hshm::u8> data_;  // Data to write (input) / verification data (output)
  OUT chi::u32 result_code_;          // Operation result (0 = success)
  OUT chi::u64 bytes_written_;        // Number of bytes actually written

  // SHM constructor
  explicit WriteTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        block_(),
        data_(alloc),
        result_code_(0),
        bytes_written_(0) {}

  // Emplace constructor
  explicit WriteTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const Block &block,
      const std::vector<hshm::u8> &data)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kWrite),
        block_(block),
        data_(alloc),
        result_code_(0),
        bytes_written_(0) {
    // Copy data from std::vector to chi::vector
    data_.resize(data.size());
    std::copy(data.begin(), data.end(), data_.begin());
  }
};

/**
 * Block read operation task
 */
struct ReadTask : public chi::Task {
  IN Block block_;                    // Source block for reading
  OUT chi::vector<hshm::u8> data_;    // Read data (output)
  OUT chi::u32 result_code_;          // Operation result (0 = success)
  OUT chi::u64 bytes_read_;           // Number of bytes actually read

  // SHM constructor
  explicit ReadTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        block_(),
        data_(alloc),
        result_code_(0),
        bytes_read_(0) {}

  // Emplace constructor
  explicit ReadTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const Block &block)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kRead),
        block_(block),
        data_(alloc),
        result_code_(0),
        bytes_read_(0) {}
};

/**
 * Performance statistics retrieval task
 */
struct StatTask : public chi::Task {
  OUT PerfMetrics metrics_;           // Performance metrics (output)
  OUT chi::u64 remaining_size_;       // Remaining allocatable space (output)
  OUT chi::u32 result_code_;          // Operation result (0 = success)

  // SHM constructor
  explicit StatTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        metrics_(),
        remaining_size_(0),
        result_code_(0) {}

  // Emplace constructor
  explicit StatTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kGetStats),
        metrics_(),
        remaining_size_(0),
        result_code_(0) {}
};

// ==============================================================================
// Target Registration Tasks
// ==============================================================================

/**
 * Target registration task
 * Registers a target where target name equals bdev pool name
 */
struct RegisterTargetTask : public chi::Task {
  IN chi::string target_name_;        // Target name (must match bdev pool name)
  OUT chi::u32 result_code_;          // Operation result (0 = success)
  OUT chi::string error_message_;     // Error description if failed

  // SHM constructor
  explicit RegisterTargetTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        target_name_(alloc),
        result_code_(0),
        error_message_(alloc) {}

  // Emplace constructor
  explicit RegisterTargetTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const std::string &target_name)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kRegisterTarget),
        target_name_(alloc, target_name),
        result_code_(0),
        error_message_(alloc) {}
};

/**
 * Target unregistration task
 * Unregisters a target by name
 */
struct UnregisterTargetTask : public chi::Task {
  IN chi::string target_name_;        // Target name to unregister
  OUT chi::u32 result_code_;          // Operation result (0 = success)
  OUT chi::string error_message_;     // Error description if failed

  // SHM constructor
  explicit UnregisterTargetTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        target_name_(alloc),
        result_code_(0),
        error_message_(alloc) {}

  // Emplace constructor
  explicit UnregisterTargetTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const std::string &target_name)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kUnregisterTarget),
        target_name_(alloc, target_name),
        result_code_(0),
        error_message_(alloc) {}
};

/**
 * Target listing task
 * Lists all registered targets
 */
struct ListTargetsTask : public chi::Task {
  OUT chi::vector<chi::string> targets_;  // List of registered target names
  OUT chi::u32 result_code_;              // Operation result (0 = success)
  OUT chi::string error_message_;         // Error description if failed

  // SHM constructor
  explicit ListTargetsTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        targets_(alloc),
        result_code_(0),
        error_message_(alloc) {}

  // Emplace constructor
  explicit ListTargetsTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kListTargets),
        targets_(alloc),
        result_code_(0),
        error_message_(alloc) {}
};

}  // namespace chimaera::bdev_extended

#endif  // BDEV_TASKS_H_