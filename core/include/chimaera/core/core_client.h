#ifndef WRPCTE_CORE_CLIENT_H_
#define WRPCTE_CORE_CLIENT_H_

#include <chimaera/chimaera.h>
#include <chimaera/core/core_tasks.h>
#include <hermes_shm/util/singleton.h>

namespace wrp_cte::core {

class Client : public chi::ContainerClient {
 public:
  Client() = default;
  explicit Client(const chi::PoolId& pool_id) { Init(pool_id); }

  /**
   * Synchronous container creation - waits for completion
   */
  void Create(const hipc::MemContext& mctx, 
              const chi::PoolQuery& pool_query,
              const CreateParams& params = CreateParams()) {
    auto task = AsyncCreate(mctx, pool_query, params);
    task->Wait();
    
    // Check if CreateTask succeeded and update client's pool_id_ to the actual pool created/found
    // This is required because CreateTask is a GetOrCreatePoolTask that may return a different
    // pool ID than what was requested if the pool already existed
    if (task->return_code_ == 0) {
      pool_id_ = task->new_pool_id_;
    }
    
    CHI_IPC->DelTask(task);
  }

  /**
   * Asynchronous container creation - returns immediately
   */
  hipc::FullPtr<CreateTask> AsyncCreate(
      const hipc::MemContext& mctx,
      const chi::PoolQuery& pool_query,
      const CreateParams& params = CreateParams()) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    // CRITICAL: CreateTask MUST use admin pool for GetOrCreatePool processing
    auto task = ipc_manager->NewTask<CreateTask>(
        chi::CreateTaskNode(),
        chi::kAdminPoolId,  // Always use admin pool for CreateTask
        pool_query,
        "wrp_cte_core",     // ChiMod name
        std::to_string(pool_id_.ToU64()),   // Pool name as string
        pool_id_,          // Target pool ID
        params);            // CreateParams with configuration
    
    // Submit to runtime
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target registration - waits for completion
   */
  chi::u32 RegisterTarget(const hipc::MemContext& mctx, 
                         const std::string& target_name,
                         chimaera::bdev::BdevType bdev_type,
                         chi::u64 total_size) {
    auto task = AsyncRegisterTarget(mctx, target_name, bdev_type, total_size);
    task->Wait();
    chi::u32 result = task->result_code_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target registration - returns immediately
   */
  hipc::FullPtr<RegisterTargetTask> AsyncRegisterTarget(
      const hipc::MemContext& mctx,
      const std::string& target_name,
      chimaera::bdev::BdevType bdev_type,
      chi::u64 total_size) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<RegisterTargetTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        target_name,
        bdev_type,
        total_size);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target unregistration - waits for completion
   */
  chi::u32 UnregisterTarget(const hipc::MemContext& mctx, 
                           const std::string& target_name) {
    auto task = AsyncUnregisterTarget(mctx, target_name);
    task->Wait();
    chi::u32 result = task->result_code_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target unregistration - returns immediately
   */
  hipc::FullPtr<UnregisterTargetTask> AsyncUnregisterTarget(
      const hipc::MemContext& mctx,
      const std::string& target_name) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<UnregisterTargetTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        target_name);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target listing - waits for completion
   */
  std::vector<TargetInfo> ListTargets(const hipc::MemContext& mctx) {
    auto task = AsyncListTargets(mctx);
    task->Wait();
    
    // Convert HSHM vector to standard vector for client use
    std::vector<TargetInfo> result;
    result.reserve(task->targets_.size());
    for (const auto& target : task->targets_) {
      result.push_back(target);
    }
    
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target listing - returns immediately
   */
  hipc::FullPtr<ListTargetsTask> AsyncListTargets(
      const hipc::MemContext& mctx) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<ListTargetsTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local());
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target stats update - waits for completion
   */
  chi::u32 StatTargets(const hipc::MemContext& mctx) {
    auto task = AsyncStatTargets(mctx);
    task->Wait();
    chi::u32 result = task->result_code_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target stats update - returns immediately
   */
  hipc::FullPtr<StatTargetsTask> AsyncStatTargets(
      const hipc::MemContext& mctx) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<StatTargetsTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local());
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get or create tag - waits for completion
   */
  TagInfo GetOrCreateTag(const hipc::MemContext& mctx,
                        const std::string& tag_name,
                        const TagId& tag_id = TagId::GetNull()) {
    auto task = AsyncGetOrCreateTag(mctx, tag_name, tag_id);
    task->Wait();
    
    TagInfo result = task->tag_info_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get or create tag - returns immediately
   */
  hipc::FullPtr<GetOrCreateTagTask<CreateParams>> AsyncGetOrCreateTag(
      const hipc::MemContext& mctx,
      const std::string& tag_name,
      const TagId& tag_id = TagId::GetNull()) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<GetOrCreateTagTask<CreateParams>>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_name,
        tag_id);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous put blob - waits for completion
   */
  bool PutBlob(const hipc::MemContext& mctx,
              const TagId& tag_id,
              const std::string& blob_name,
              const BlobId& blob_id,
              chi::u64 offset,
              chi::u64 size,
              hipc::Pointer blob_data,
              float score,
              chi::u32 flags) {
    auto task = AsyncPutBlob(mctx, tag_id, blob_name, blob_id, 
                            offset, size, blob_data, score, flags);
    task->Wait();
    bool result = (task->result_code_ == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous put blob - returns immediately (unimplemented for now)
   */
  hipc::FullPtr<PutBlobTask> AsyncPutBlob(
      const hipc::MemContext& mctx,
      const TagId& tag_id,
      const std::string& blob_name,
      const BlobId& blob_id,
      chi::u64 offset,
      chi::u64 size,
      hipc::Pointer blob_data,
      float score,
      chi::u32 flags) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<PutBlobTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_id,
        blob_name,
        blob_id,
        offset,
        size,
        blob_data,
        score,
        flags);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get blob - waits for completion
   */
  bool GetBlob(const hipc::MemContext& mctx,
              const TagId& tag_id,
              const std::string& blob_name,
              const BlobId& blob_id,
              chi::u64 offset,
              chi::u64 size,
              chi::u32 flags,
              hipc::Pointer blob_data) {
    auto task = AsyncGetBlob(mctx, tag_id, blob_name, blob_id, 
                            offset, size, flags, blob_data);
    task->Wait();
    bool result = (task->result_code_ == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get blob - returns immediately
   */
  hipc::FullPtr<GetBlobTask> AsyncGetBlob(
      const hipc::MemContext& mctx,
      const TagId& tag_id,
      const std::string& blob_name,
      const BlobId& blob_id,
      chi::u64 offset,
      chi::u64 size,
      chi::u32 flags,
      hipc::Pointer blob_data) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<GetBlobTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_id,
        blob_name,
        blob_id,
        offset,
        size,
        flags,
        blob_data);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous reorganize blob - waits for completion (unimplemented for now)
   */
  chi::u32 ReorganizeBlob(const hipc::MemContext& mctx,
                         const BlobId& blob_id,
                         float new_score) {
    auto task = AsyncReorganizeBlob(mctx, blob_id, new_score);
    task->Wait();
    chi::u32 result = task->result_code_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous reorganize blob - returns immediately (unimplemented for now)
   */
  hipc::FullPtr<ReorganizeBlobTask> AsyncReorganizeBlob(
      const hipc::MemContext& mctx,
      const BlobId& blob_id,
      float new_score) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<ReorganizeBlobTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        blob_id,
        new_score);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous delete blob - waits for completion
   */
  bool DelBlob(const hipc::MemContext& mctx,
              const TagId& tag_id,
              const std::string& blob_name,
              const BlobId& blob_id) {
    auto task = AsyncDelBlob(mctx, tag_id, blob_name, blob_id);
    task->Wait();
    bool result = (task->result_code_ == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous delete blob - returns immediately
   */
  hipc::FullPtr<DelBlobTask> AsyncDelBlob(
      const hipc::MemContext& mctx,
      const TagId& tag_id,
      const std::string& blob_name,
      const BlobId& blob_id) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<DelBlobTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_id,
        blob_name,
        blob_id);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous delete tag by tag ID - waits for completion
   */
  bool DelTag(const hipc::MemContext& mctx,
             const TagId& tag_id) {
    auto task = AsyncDelTag(mctx, tag_id);
    task->Wait();
    bool result = (task->result_code_ == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Synchronous delete tag by tag name - waits for completion
   */
  bool DelTag(const hipc::MemContext& mctx,
             const std::string& tag_name) {
    auto task = AsyncDelTag(mctx, tag_name);
    task->Wait();
    bool result = (task->result_code_ == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous delete tag by tag ID - returns immediately
   */
  hipc::FullPtr<DelTagTask> AsyncDelTag(
      const hipc::MemContext& mctx,
      const TagId& tag_id) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<DelTagTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_id);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Asynchronous delete tag by tag name - returns immediately
   */
  hipc::FullPtr<DelTagTask> AsyncDelTag(
      const hipc::MemContext& mctx,
      const std::string& tag_name) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<DelTagTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_name);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get tag size - waits for completion
   */
  size_t GetTagSize(const hipc::MemContext& mctx,
                   const TagId& tag_id) {
    auto task = AsyncGetTagSize(mctx, tag_id);
    task->Wait();
    size_t result = (task->result_code_ == 0) ? task->tag_size_ : 0;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get tag size - returns immediately
   */
  hipc::FullPtr<GetTagSizeTask> AsyncGetTagSize(
      const hipc::MemContext& mctx,
      const TagId& tag_id) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<GetTagSizeTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        tag_id);
    
    ipc_manager->Enqueue(task);
    return task;
  }
};

// Forward declaration for Config
class Config;

// Global pointer-based singletons with lazy initialization
HSHM_DEFINE_GLOBAL_PTR_VAR_H(wrp_cte::core::Client, g_cte_client);
HSHM_DEFINE_GLOBAL_PTR_VAR_H(wrp_cte::core::Config, g_cte_config);

/**
 * Initialize CTE client and configuration subsystem
 * @param config_path Optional path to configuration file
 * @return true if initialization succeeded, false otherwise
 */
bool WRP_CTE_INIT(const std::string& config_path = "");

}  // namespace wrp_cte::core

// Global singleton macros for easy access (return pointers, not references)
#define WRP_CTE_CLIENT (&(*HSHM_GET_GLOBAL_PTR_VAR(wrp_cte::core::Client, wrp_cte::core::g_cte_client)))
#define WRP_CTE_CONFIG (&(*HSHM_GET_GLOBAL_PTR_VAR(wrp_cte::core::Config, wrp_cte::core::g_cte_config)))

#endif  // WRPCTE_CORE_CLIENT_H_