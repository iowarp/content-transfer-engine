#ifndef WRPCTE_CORE_RUNTIME_H_
#define WRPCTE_CORE_RUNTIME_H_

#include <chimaera/chimaera.h>
#include <chimaera/core/core_tasks.h>
#include <chimaera/core/core_client.h>
#include <chimaera/core/core_config.h>
#include <chimaera/comutex.h>
#include <chimaera/corwlock.h>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

// Forward declarations to avoid circular dependency
namespace wrp_cte::core {
  class Config;
}

namespace wrp_cte::core {

/**
 * CTE Core Runtime Container
 * Implements target management and tag/blob operations
 */
class Runtime : public chi::Container {
 public:
  using CreateParams = wrp_cte::core::CreateParams;  // Required for CHI_TASK_CC
  
  Runtime() = default;
  ~Runtime() override = default;

  /**
   * Initialize client for this container (REQUIRED)
   */
  void InitClient(const chi::PoolId& pool_id) {
    client_ = Client(pool_id);
  }

  /**
   * Create the container (Method::kCreate)
   * This method both creates and initializes the container
   */
  void Create(hipc::FullPtr<CreateTask> task, chi::RunContext& ctx);

  /**
   * Monitor create progress
   */
  void MonitorCreate(chi::MonitorModeId mode, hipc::FullPtr<CreateTask> task,
                     chi::RunContext& ctx);

  /**
   * Destroy the container (Method::kDestroy)
   */
  void Destroy(hipc::FullPtr<DestroyTask> task, chi::RunContext& ctx);

  /**
   * Monitor destroy progress
   */
  void MonitorDestroy(chi::MonitorModeId mode, hipc::FullPtr<DestroyTask> task,
                      chi::RunContext& ctx);

  /**
   * Register a target (Method::kRegisterTarget)
   */
  void RegisterTarget(hipc::FullPtr<RegisterTargetTask> task, chi::RunContext& ctx);

  /**
   * Monitor register target operation
   */
  void MonitorRegisterTarget(chi::MonitorModeId mode, 
                           hipc::FullPtr<RegisterTargetTask> task,
                           chi::RunContext& ctx);

  /**
   * Unregister a target (Method::kUnregisterTarget)
   */
  void UnregisterTarget(hipc::FullPtr<UnregisterTargetTask> task, chi::RunContext& ctx);

  /**
   * Monitor unregister target operation
   */
  void MonitorUnregisterTarget(chi::MonitorModeId mode, 
                             hipc::FullPtr<UnregisterTargetTask> task,
                             chi::RunContext& ctx);

  /**
   * List registered targets (Method::kListTargets)
   */
  void ListTargets(hipc::FullPtr<ListTargetsTask> task, chi::RunContext& ctx);

  /**
   * Monitor list targets operation
   */
  void MonitorListTargets(chi::MonitorModeId mode, 
                        hipc::FullPtr<ListTargetsTask> task,
                        chi::RunContext& ctx);

  /**
   * Update target statistics (Method::kStatTargets)
   */
  void StatTargets(hipc::FullPtr<StatTargetsTask> task, chi::RunContext& ctx);

  /**
   * Monitor stat targets operation
   */
  void MonitorStatTargets(chi::MonitorModeId mode, 
                        hipc::FullPtr<StatTargetsTask> task,
                        chi::RunContext& ctx);

  /**
   * Get or create a tag (Method::kGetOrCreateTag)
   */
  template<typename CreateParamsT = CreateParams>
  void GetOrCreateTag(hipc::FullPtr<GetOrCreateTagTask<CreateParamsT>> task, chi::RunContext& ctx);

  /**
   * Monitor get or create tag operation
   */
  template<typename CreateParamsT = CreateParams>
  void MonitorGetOrCreateTag(chi::MonitorModeId mode, 
                           hipc::FullPtr<GetOrCreateTagTask<CreateParamsT>> task,
                           chi::RunContext& ctx);

  /**
   * Put blob (Method::kPutBlob) - unimplemented for now
   */
  void PutBlob(hipc::FullPtr<PutBlobTask> task, chi::RunContext& ctx);

  /**
   * Monitor put blob operation
   */
  void MonitorPutBlob(chi::MonitorModeId mode, 
                    hipc::FullPtr<PutBlobTask> task,
                    chi::RunContext& ctx);

  /**
   * Get blob (Method::kGetBlob) - unimplemented for now
   */
  void GetBlob(hipc::FullPtr<GetBlobTask> task, chi::RunContext& ctx);

  /**
   * Monitor get blob operation
   */
  void MonitorGetBlob(chi::MonitorModeId mode, 
                    hipc::FullPtr<GetBlobTask> task,
                    chi::RunContext& ctx);

  /**
   * Reorganize blob (Method::kReorganizeBlob) - unimplemented for now
   */
  void ReorganizeBlob(hipc::FullPtr<ReorganizeBlobTask> task, chi::RunContext& ctx);

  /**
   * Monitor reorganize blob operation
   */
  void MonitorReorganizeBlob(chi::MonitorModeId mode, 
                           hipc::FullPtr<ReorganizeBlobTask> task,
                           chi::RunContext& ctx);

  /**
   * Delete blob operation - removes blob and decrements tag size
   */
  void DelBlob(hipc::FullPtr<DelBlobTask> task, chi::RunContext& ctx);

  /**
   * Monitor delete blob operation
   */
  void MonitorDelBlob(chi::MonitorModeId mode, 
                      hipc::FullPtr<DelBlobTask> task,
                      chi::RunContext& ctx);

  /**
   * Delete tag operation - removes all blobs from tag and removes tag
   */
  void DelTag(hipc::FullPtr<DelTagTask> task, chi::RunContext& ctx);

  /**
   * Monitor delete tag operation
   */
  void MonitorDelTag(chi::MonitorModeId mode, 
                     hipc::FullPtr<DelTagTask> task,
                     chi::RunContext& ctx);

  /**
   * Get tag size operation - returns total size of all blobs in tag
   */
  void GetTagSize(hipc::FullPtr<GetTagSizeTask> task, chi::RunContext& ctx);

  /**
   * Monitor get tag size operation
   */
  void MonitorGetTagSize(chi::MonitorModeId mode, 
                         hipc::FullPtr<GetTagSizeTask> task,
                         chi::RunContext& ctx);

  // Pure virtual methods - implementations are in autogen/core_lib_exec.cc
  void Run(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr, chi::RunContext& rctx) override;
  void Monitor(chi::MonitorModeId mode, chi::u32 method, 
               hipc::FullPtr<chi::Task> task_ptr, chi::RunContext& rctx) override;
  void Del(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr) override;
  chi::u64 GetWorkRemaining() const override;
  void SaveIn(chi::u32 method, chi::TaskSaveInArchive& archive,
              hipc::FullPtr<chi::Task> task_ptr) override;
  void LoadIn(chi::u32 method, chi::TaskLoadInArchive& archive,
              hipc::FullPtr<chi::Task> task_ptr) override;
  void SaveOut(chi::u32 method, chi::TaskSaveOutArchive& archive,
               hipc::FullPtr<chi::Task> task_ptr) override;
  void LoadOut(chi::u32 method, chi::TaskLoadOutArchive& archive,
               hipc::FullPtr<chi::Task> task_ptr) override;
  void NewCopy(chi::u32 method, const hipc::FullPtr<chi::Task> &orig_task,
               hipc::FullPtr<chi::Task> &dup_task, bool deep) override;
  // Autogen-expected signatures (older API)
  void SaveIn(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  void LoadIn(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  void SaveOut(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  void LoadOut(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  hipc::FullPtr<chi::Task> NewCopy(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);

 private:
  // Queue ID constants (REQUIRED: Use semantic names, not raw integers)
  static const chi::QueueId kTargetManagementQueue = 0;
  static const chi::QueueId kTagManagementQueue = 1;
  static const chi::QueueId kBlobOperationsQueue = 2;
  static const chi::QueueId kStatsQueue = 3;

  // Client for this ChiMod
  Client client_;

  // Target management data structures
  std::unordered_map<chi::PoolId, TargetInfo> registered_targets_;
  std::unordered_map<std::string, chi::PoolId> target_name_to_id_;  // reverse lookup: target_name -> target_id
  
  // Tag management data structures
  std::unordered_map<std::string, TagId> tag_name_to_id_;     // tag_name -> tag_id
  std::unordered_map<TagId, TagInfo> tag_id_to_info_;         // tag_id -> TagInfo
  std::unordered_map<std::string, BlobId> tag_blob_name_to_id_;  // "tag_id.blob_name" -> blob_id
  std::unordered_map<BlobId, BlobInfo> blob_id_to_info_;      // blob_id -> BlobInfo
  
  // Atomic counters for thread-safe ID generation
  std::atomic<chi::u32> next_tag_id_minor_;   // Minor counter for TagId UniqueId generation
  std::atomic<chi::u32> next_blob_id_minor_;  // Minor counter for BlobId UniqueId generation

  // Synchronization primitives for thread-safe access to data structures
  // Use a set of locks based on maximum number of lanes for better concurrency
  static const size_t kMaxLocks = 64;  // Maximum number of locks (matches max lanes)
  std::vector<std::unique_ptr<chi::CoRwLock>> target_locks_;   // For registered_targets_
  std::vector<std::unique_ptr<chi::CoRwLock>> tag_locks_;      // For tag management structures

  // Storage configuration (parsed from config file)
  std::vector<StorageDeviceConfig> storage_devices_;

  /**
   * Get access to configuration manager
   */
  const Config& GetConfig() const;


  /**
   * Helper function to update target performance statistics
   */
  void UpdateTargetStats(const chi::PoolId& target_id, TargetInfo& target_info);

  /**
   * Helper function to get or assign a tag ID
   */
  TagId GetOrAssignTagId(const std::string& tag_name, const TagId& preferred_id = TagId::GetNull());

  /**
   * Helper function to generate a new TagId using node_id as major and atomic counter as minor
   */
  TagId GenerateNewTagId();

  /**
   * Helper function to generate a new BlobId using node_id as major and atomic counter as minor
   */
  BlobId GenerateNewBlobId();

  /**
   * Helper function to get or assign a blob ID
   */
  BlobId GetOrAssignBlobId(const TagId& tag_id, const std::string& blob_name, 
                          const BlobId& preferred_id = BlobId::GetNull());
  
  /**
   * Get target lock index based on TargetId hash
   */
  size_t GetTargetLockIndex(const chi::PoolId& target_id) const;
  
  /**
   * Get tag lock index based on tag name hash
   */
  size_t GetTagLockIndex(const std::string& tag_name) const;
  
  /**
   * Get blob lock index based on blob ID hash
   */
  size_t GetBlobLockIndex(const BlobId& blob_id) const;
  
  /**
   * Allocate space from a target for new blob data
   * @param target_info Target to allocate from
   * @param size Size to allocate
   * @param allocated_offset Output parameter for allocated offset
   * @return True if allocation succeeded, false otherwise
   */
  bool AllocateFromTarget(TargetInfo& target_info, chi::u64 size, 
                         chi::u64& allocated_offset);

  /**
   * Check if blob exists and return pointer to BlobInfo if found
   * @param blob_id BlobId to search for (can be null)
   * @param blob_name Blob name to search for (can be empty)
   * @param tag_id Tag ID to search within
   * @param found_blob_id Output parameter for the actual blob ID found
   * @return Pointer to BlobInfo if found, nullptr if not found
   */
  BlobInfo* CheckBlobExists(const BlobId& blob_id, const std::string& blob_name, 
                           const TagId& tag_id, BlobId& found_blob_id);

  /**
   * Create new blob with given parameters
   * @param blob_name Name for the new blob (required)
   * @param tag_id Tag ID to associate blob with
   * @param blob_score Score/priority for the blob
   * @param created_blob_id Output parameter for the generated blob ID
   * @return Pointer to created BlobInfo, nullptr on failure
   */
  BlobInfo* CreateNewBlob(const std::string& blob_name, const TagId& tag_id, 
                         float blob_score, BlobId& created_blob_id);

  /**
   * Allocate new data blocks for blob expansion
   * @param blob_info Blob to extend with new data blocks
   * @param offset Offset where data starts (for determining required size)
   * @param size Size of data to accommodate
   * @param blob_score Score for target selection
   * @return Error code: 0 for success, 1 for failure
   */
  chi::u32 AllocateNewData(BlobInfo& blob_info, chi::u64 offset, chi::u64 size, 
                          float blob_score);

  /**
   * Write data to existing blob blocks
   * @param blob_info Blob containing the blocks to write to
   * @param offset Offset within blob where data starts
   * @param size Size of data to write
   * @param blob_data Pointer to data to write
   * @return Error code: 0 for success, 1 for failure
   */
  chi::u32 ModifyExistingData(const std::vector<BlobBlock> &blocks, 
                             hipc::Pointer data, size_t data_size, 
                             size_t data_offset_in_blob);
  
  /**
   * Read existing blob data from blocks
   * @param blocks Vector of blob blocks to read from
   * @param data Output buffer to read data into
   * @param data_size Size of data to read
   * @param data_offset_in_blob Offset within blob where reading starts
   * @return Error code: 0 for success, 1 for failure
   */
  chi::u32 ReadData(const std::vector<BlobBlock> &blocks, 
                   hipc::Pointer data, size_t data_size, 
                   size_t data_offset_in_blob);
};

}  // namespace wrp_cte::core

#endif  // WRPCTE_CORE_RUNTIME_H_