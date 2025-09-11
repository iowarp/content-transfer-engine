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
  void GetOrCreateTag(hipc::FullPtr<GetOrCreateTagTask> task, chi::RunContext& ctx);

  /**
   * Monitor get or create tag operation
   */
  void MonitorGetOrCreateTag(chi::MonitorModeId mode, 
                           hipc::FullPtr<GetOrCreateTagTask> task,
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
  std::unordered_map<std::string, TargetInfo> registered_targets_;
  
  // Tag management data structures
  std::unordered_map<std::string, chi::u32> tag_name_to_id_;  // tag_name -> tag_id
  std::unordered_map<chi::u32, TagInfo> tag_id_to_info_;      // tag_id -> TagInfo
  std::unordered_map<chi::u32, std::unordered_map<std::string, chi::u32>> tag_blob_name_to_id_;  // tag_id.blob_name -> blob_id
  std::unordered_map<chi::u32, BlobInfo> blob_id_to_info_;    // blob_id -> BlobInfo
  
  // Atomic counters for thread-safe ID generation
  std::atomic<chi::u32> next_tag_id_;
  std::atomic<chi::u32> next_blob_id_;

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
   * Helper function to create a bdev client for target registration
   */
  std::string CreateBdevForTarget(const std::string& target_name, 
                                 const StorageDeviceConfig& device_config);

  /**
   * Helper function to update target performance statistics
   */
  void UpdateTargetStats(const std::string& target_name, TargetInfo& target_info);

  /**
   * Helper function to get or assign a tag ID
   */
  chi::u32 GetOrAssignTagId(const std::string& tag_name, chi::u32 preferred_id = 0);

  /**
   * Helper function to get or assign a blob ID
   */
  chi::u32 GetOrAssignBlobId(chi::u32 tag_id, const std::string& blob_name, 
                            chi::u32 preferred_id = 0);
  
  /**
   * Get target lock index based on target name hash
   */
  size_t GetTargetLockIndex(const std::string& target_name) const;
  
  /**
   * Get tag lock index based on tag name hash
   */
  size_t GetTagLockIndex(const std::string& tag_name) const;
};

}  // namespace wrp_cte::core

#endif  // WRPCTE_CORE_RUNTIME_H_