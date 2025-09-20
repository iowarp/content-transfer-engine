#ifndef WRPCTE_CORE_TASKS_H_
#define WRPCTE_CORE_TASKS_H_

#include <chimaera/chimaera.h>
#include <chimaera/core/autogen/core_methods.h>
// Include admin tasks for GetOrCreatePoolTask
#include <chimaera/admin/admin_tasks.h>
// Include bdev tasks for BdevType enum
#include <chimaera/bdev/bdev_tasks.h>
// Include bdev client for TargetInfo
#include <chimaera/bdev/bdev_client.h>

namespace wrp_cte::core {

/**
 * CreateParams for CTE Core chimod
 * Contains configuration parameters for CTE container creation
 */
struct CreateParams {
  // CTE-specific parameters
  chi::string config_file_path_;  // YAML config file path
  chi::u32 worker_count_;         // Number of worker threads
  
  // Required: chimod library name for module manager
  static constexpr const char* chimod_lib_name = "wrp_cte_core";
  
  // Default constructor
  CreateParams() : worker_count_(4) {}
  
  // Constructor with allocator and parameters
  CreateParams(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc, 
               const std::string& config_file_path = "", 
               chi::u32 worker_count = 4)
      : config_file_path_(alloc, config_file_path), worker_count_(worker_count) {}
      
  // Copy constructor with allocator (required for task creation)
  CreateParams(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
               const CreateParams& other)
      : config_file_path_(alloc, other.config_file_path_.str()), 
        worker_count_(other.worker_count_) {}
  
  // Constructor with allocator, pool_id, and CreateParams (required for admin task creation)
  CreateParams(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
               const chi::PoolId& pool_id,
               const CreateParams& other)
      : config_file_path_(alloc, other.config_file_path_.str()), 
        worker_count_(other.worker_count_) {
    // pool_id is used by the admin task framework, but we don't need to store it
    (void)pool_id;  // Suppress unused parameter warning
  }
  
  // Serialization support for cereal
  template<class Archive>
  void serialize(Archive& ar) {
    ar(config_file_path_, worker_count_);
  }
};

/**
 * CreateTask - Initialize the CTE Core container
 * Type alias for GetOrCreatePoolTask with CreateParams (uses kGetOrCreatePool method)
 * Non-admin modules should use GetOrCreatePoolTask instead of BaseCreateTask
 */
using CreateTask = chimaera::admin::GetOrCreatePoolTask<CreateParams>;

/**
 * DestroyTask - Destroy the CTE Core container
 * Type alias for DestroyPoolTask (uses kDestroy method)
 */
using DestroyTask = chimaera::admin::DestroyTask;

/**
 * Target information structure
 */
struct TargetInfo {
  std::string target_name_;
  std::string bdev_pool_name_;
  chimaera::bdev::Client bdev_client_;  // Bdev client for this target
  chi::u64 bytes_read_;
  chi::u64 bytes_written_;
  chi::u64 ops_read_;
  chi::u64 ops_written_;
  float target_score_;                  // Target score (0-1, normalized log bandwidth)
  chi::u64 remaining_space_;           // Remaining allocatable space in bytes
  chimaera::bdev::PerfMetrics perf_metrics_;  // Performance metrics from bdev

  TargetInfo() = default;
  
  explicit TargetInfo(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : bytes_read_(0), bytes_written_(0), ops_read_(0), ops_written_(0), 
        target_score_(0.0f), remaining_space_(0) {
    // std::string doesn't need allocator, chi::u64 and float are POD types
    (void)alloc; // Suppress unused parameter warning
  }
};

/**
 * RegisterTarget task - Get/create bdev locally, create Target struct
 */
struct RegisterTargetTask : public chi::Task {
  // Task-specific data using HSHM macros
  IN chi::string target_name_;        // Name and file path of the target to register
  IN chimaera::bdev::BdevType bdev_type_;  // Block device type enum
  IN chi::u64 total_size_;            // Total size for allocation
  OUT chi::u32 result_code_;          // Output result (0 = success)

  // SHM constructor
  explicit RegisterTargetTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        target_name_(alloc), 
        bdev_type_(chimaera::bdev::BdevType::kFile),
        total_size_(0),
        result_code_(0) {}

  // Emplace constructor
  explicit RegisterTargetTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const std::string &target_name,
      chimaera::bdev::BdevType bdev_type,
      chi::u64 total_size)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kRegisterTarget),
        target_name_(alloc, target_name),
        bdev_type_(bdev_type),
        total_size_(total_size),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kRegisterTarget;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * UnregisterTarget task - Unlink bdev from container (don't destroy bdev container)
 */
struct UnregisterTargetTask : public chi::Task {
  IN chi::string target_name_;        // Name of the target to unregister
  OUT chi::u32 result_code_;          // Output result (0 = success)

  // SHM constructor
  explicit UnregisterTargetTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        target_name_(alloc), 
        result_code_(0) {}

  // Emplace constructor
  explicit UnregisterTargetTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const std::string &target_name)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kUnregisterTarget),
        target_name_(alloc, target_name),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kUnregisterTarget;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * ListTargets task - Return set of registered targets on this node
 */
struct ListTargetsTask : public chi::Task {
  OUT chi::vector<TargetInfo> targets_;  // List of registered targets
  OUT chi::u32 result_code_;             // Output result (0 = success)

  // SHM constructor
  explicit ListTargetsTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        targets_(alloc),
        result_code_(0) {}

  // Emplace constructor
  explicit ListTargetsTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kListTargets),
        targets_(alloc),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kListTargets;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * StatTargets task - Poll each target in vector, update performance stats
 */
struct StatTargetsTask : public chi::Task {
  OUT chi::u32 result_code_;          // Output result (0 = success)

  // SHM constructor
  explicit StatTargetsTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        result_code_(0) {}

  // Emplace constructor
  explicit StatTargetsTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kStatTargets),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kStatTargets;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * TagId type definition
 * Uses chi::UniqueId with node_id as major and atomic counter as minor
 */
using TagId = chi::UniqueId;

/**
 * BlobId type definition
 * Uses chi::UniqueId with node_id as major and atomic counter as minor
 */
using BlobId = chi::UniqueId;

} // namespace wrp_cte::core

// Hash specialization for TagId (TagId uses same hash as chi::UniqueId)
namespace hshm {
template<>
struct hash<wrp_cte::core::TagId> {
  std::size_t operator()(const wrp_cte::core::TagId& id) const {
    std::hash<chi::u32> hasher;
    return hasher(id.major_) ^ (hasher(id.minor_) << 1);
  }
};
} // namespace hshm

namespace wrp_cte::core {

/**
 * Tag information structure for blob grouping
 */
struct TagInfo {
  std::string tag_name_;
  TagId tag_id_;
  std::unordered_map<BlobId, chi::u32> blob_ids_;  // Map of blob IDs in this tag (using as set)
  size_t total_size_;  // Total size of all blobs in this tag

  TagInfo() : tag_name_(), tag_id_(TagId::GetNull()), blob_ids_(), total_size_(0) {}
  
  explicit TagInfo(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : tag_name_(), tag_id_(TagId::GetNull()), blob_ids_(), total_size_(0) {
    (void)alloc; // Suppress unused parameter warning
  }
      
  TagInfo(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
          const std::string &tag_name, const TagId& tag_id)
      : tag_name_(tag_name), tag_id_(tag_id), blob_ids_(), total_size_(0) {
    (void)alloc; // Suppress unused parameter warning
  }
};

/**
 * Block structure for blob management
 * Each block represents a portion of a blob stored in a target
 */
struct BlobBlock {
  chimaera::bdev::Client bdev_client_; // Bdev client for this block's target
  chi::u64 target_offset_;            // Offset within target where this block is stored
  chi::u64 size_;                     // Size of this block in bytes
  
  BlobBlock() = default;
  
  BlobBlock(const chimaera::bdev::Client& client, chi::u64 offset, chi::u64 size)
      : bdev_client_(client), target_offset_(offset), size_(size) {}
};

/**
 * Blob information structure with block-based management
 */
struct BlobInfo {
  BlobId blob_id_;
  std::string blob_name_;
  std::vector<BlobBlock> blocks_;     // Vector of blocks that make up this blob (ordered)
  float score_;                       // 0-1 score for reorganization

  BlobInfo() = default;
  
  explicit BlobInfo(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : blob_id_(BlobId::GetNull()), blob_name_(), blocks_(), score_(0.0f) {
    (void)alloc; // Suppress unused parameter warning
  }
        
  BlobInfo(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
           const BlobId& blob_id, const std::string &blob_name,
           float score)
      : blob_id_(blob_id), blob_name_(blob_name), 
        blocks_(), score_(score) {
    (void)alloc; // Suppress unused parameter warning
  }
        
  /**
   * Get total size of blob by summing all block sizes
   */
  chi::u64 GetTotalSize() const {
    chi::u64 total = 0;
    for (size_t i = 0; i < blocks_.size(); ++i) {
      total += blocks_[i].size_;
    }
    return total;
  }
};

/**
 * GetOrCreateTag task - Get or create a tag for blob grouping
 * Template parameter allows different CreateParams types
 */
template<typename CreateParamsT = CreateParams>
struct GetOrCreateTagTask : public chi::Task {
  IN chi::string tag_name_;           // Tag name (required)
  INOUT TagId tag_id_;               // Tag unique ID (default null, output on creation)
  OUT TagInfo tag_info_;             // Complete tag information
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit GetOrCreateTagTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        tag_name_(alloc),
        tag_id_(TagId::GetNull()),
        tag_info_(alloc),
        result_code_(0) {}

  // Emplace constructor
  explicit GetOrCreateTagTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const std::string &tag_name,
      const TagId& tag_id = TagId::GetNull())
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kGetOrCreateTag),
        tag_name_(alloc, tag_name),
        tag_id_(tag_id),
        tag_info_(alloc),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kGetOrCreateTag;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * PutBlob task - Store a blob (unimplemented for now)
 */
struct PutBlobTask : public chi::Task {
  IN TagId tag_id_;                  // Tag ID for blob grouping
  INOUT chi::string blob_name_;       // Blob name (optional, generated if empty)
  INOUT BlobId blob_id_;             // Blob ID (optional, generated if null)
  IN chi::u64 offset_;               // Offset within blob
  IN chi::u64 size_;                 // Size of blob data
  IN hipc::Pointer blob_data_;       // Blob data (shared memory pointer)
  IN float score_;                   // Score 0-1 for placement decisions
  IN chi::u32 flags_;                // Operation flags
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit PutBlobTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        tag_id_(TagId::GetNull()),
        blob_name_(alloc),
        blob_id_(BlobId::GetNull()),
        offset_(0),
        size_(0),
        blob_data_(hipc::Pointer::GetNull()),
        score_(0.5f),
        flags_(0),
        result_code_(0) {}

  // Emplace constructor
  explicit PutBlobTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const TagId& tag_id,
      const std::string &blob_name,
      const BlobId& blob_id,
      chi::u64 offset,
      chi::u64 size,
      hipc::Pointer blob_data,
      float score,
      chi::u32 flags)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kPutBlob),
        tag_id_(tag_id),
        blob_name_(alloc, blob_name),
        blob_id_(blob_id),
        offset_(offset),
        size_(size),
        blob_data_(blob_data),
        score_(score),
        flags_(flags),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kPutBlob;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * GetBlob task - Retrieve a blob (unimplemented for now)
 */
struct GetBlobTask : public chi::Task {
  IN TagId tag_id_;                  // Tag ID for blob lookup
  INOUT chi::string blob_name_;       // Blob name (optional)
  INOUT BlobId blob_id_;             // Blob ID (optional)
  IN chi::u64 offset_;               // Offset within blob
  IN chi::u64 size_;                 // Size of data to retrieve
  IN chi::u32 flags_;                // Operation flags
  IN hipc::Pointer blob_data_;       // Input buffer for blob data (shared memory pointer)
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit GetBlobTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        tag_id_(TagId::GetNull()),
        blob_name_(alloc),
        blob_id_(BlobId::GetNull()),
        offset_(0),
        size_(0),
        flags_(0),
        blob_data_(hipc::Pointer::GetNull()),
        result_code_(0) {}

  // Emplace constructor
  explicit GetBlobTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const TagId& tag_id,
      const std::string &blob_name,
      const BlobId& blob_id,
      chi::u64 offset,
      chi::u64 size,
      chi::u32 flags,
      hipc::Pointer blob_data)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kGetBlob),
        tag_id_(tag_id),
        blob_name_(alloc, blob_name),
        blob_id_(blob_id),
        offset_(offset),
        size_(size),
        flags_(flags),
        blob_data_(blob_data),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kGetBlob;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * ReorganizeBlob task - Change blob score (unimplemented for now)
 */
struct ReorganizeBlobTask : public chi::Task {
  IN BlobId blob_id_;                // Blob ID to reorganize
  IN float new_score_;               // New score (0-1)
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit ReorganizeBlobTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        blob_id_(BlobId::GetNull()),
        new_score_(0.5f),
        result_code_(0) {}

  // Emplace constructor
  explicit ReorganizeBlobTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const BlobId& blob_id,
      float new_score)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kReorganizeBlob),
        blob_id_(blob_id),
        new_score_(new_score),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kReorganizeBlob;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * DelBlob task - Remove blob and decrement tag size
 */
struct DelBlobTask : public chi::Task {
  IN TagId tag_id_;                  // Tag ID for blob lookup
  IN chi::string blob_name_;         // Blob name (optional)
  IN BlobId blob_id_;                // Blob ID (optional)
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit DelBlobTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        tag_id_(TagId::GetNull()),
        blob_name_(alloc),
        blob_id_(BlobId::GetNull()),
        result_code_(0) {}

  // Emplace constructor
  explicit DelBlobTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const TagId& tag_id,
      const std::string &blob_name,
      const BlobId& blob_id)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kDelBlob),
        tag_id_(tag_id),
        blob_name_(alloc, blob_name),
        blob_id_(blob_id),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kDelBlob;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * DelTag task - Remove all blobs from tag and remove tag
 */
struct DelTagTask : public chi::Task {
  IN TagId tag_id_;                  // Tag ID to delete
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit DelTagTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        tag_id_(TagId::GetNull()),
        result_code_(0) {}

  // Emplace constructor
  explicit DelTagTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const TagId& tag_id)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kDelTag),
        tag_id_(tag_id),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kDelTag;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

/**
 * GetTagSize task - Get the total size of a tag
 */
struct GetTagSizeTask : public chi::Task {
  IN TagId tag_id_;                  // Tag ID to query
  OUT size_t tag_size_;              // Total size of all blobs in tag
  OUT chi::u32 result_code_;         // Output result (0 = success)

  // SHM constructor
  explicit GetTagSizeTask(const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc)
      : chi::Task(alloc), 
        tag_id_(TagId::GetNull()),
        tag_size_(0),
        result_code_(0) {}

  // Emplace constructor
  explicit GetTagSizeTask(
      const hipc::CtxAllocator<CHI_MAIN_ALLOC_T> &alloc,
      const chi::TaskNode &task_node,
      const chi::PoolId &pool_id,
      const chi::PoolQuery &pool_query,
      const TagId& tag_id)
      : chi::Task(alloc, task_node, pool_id, pool_query, Method::kGetTagSize),
        tag_id_(tag_id),
        tag_size_(0),
        result_code_(0) {
    task_node_ = task_node;
    pool_id_ = pool_id;
    method_ = Method::kGetTagSize;
    task_flags_.Clear();
    pool_query_ = pool_query;
  }
};

}  // namespace wrp_cte::core

#endif  // WRPCTE_CORE_TASKS_H_