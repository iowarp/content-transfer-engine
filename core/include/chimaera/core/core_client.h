#ifndef WRPCTE_CORE_CLIENT_H_
#define WRPCTE_CORE_CLIENT_H_

#include <chimaera/chimaera.h>
#include <chimaera/core/core_tasks.h>

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
                         const std::string& file_path,
                         chi::u64 total_size) {
    auto task = AsyncRegisterTarget(mctx, target_name, bdev_type, file_path, total_size);
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
      const std::string& file_path,
      chi::u64 total_size) {
    (void)mctx;  // Suppress unused parameter warning
    auto* ipc_manager = CHI_IPC;
    
    auto task = ipc_manager->NewTask<RegisterTargetTask>(
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        target_name,
        bdev_type,
        file_path,
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
                        chi::u32 tag_id = 0) {
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
      chi::u32 tag_id = 0) {
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
   * Synchronous put blob - waits for completion (unimplemented for now)
   */
  chi::u32 PutBlob(const hipc::MemContext& mctx,
                  chi::u32 tag_id,
                  const std::string& blob_name,
                  chi::u32 blob_id,
                  chi::u64 offset,
                  chi::u64 size,
                  hipc::Pointer blob_data,
                  float score,
                  chi::u32 flags) {
    auto task = AsyncPutBlob(mctx, tag_id, blob_name, blob_id, 
                            offset, size, blob_data, score, flags);
    task->Wait();
    chi::u32 result = task->result_code_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous put blob - returns immediately (unimplemented for now)
   */
  hipc::FullPtr<PutBlobTask> AsyncPutBlob(
      const hipc::MemContext& mctx,
      chi::u32 tag_id,
      const std::string& blob_name,
      chi::u32 blob_id,
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
   * Synchronous get blob - waits for completion (unimplemented for now)
   */
  hipc::Pointer GetBlob(const hipc::MemContext& mctx,
                       chi::u32 tag_id,
                       const std::string& blob_name,
                       chi::u32 blob_id,
                       chi::u64 offset,
                       chi::u64 size,
                       chi::u32 flags) {
    auto task = AsyncGetBlob(mctx, tag_id, blob_name, blob_id, 
                            offset, size, flags);
    task->Wait();
    hipc::Pointer result = task->blob_data_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get blob - returns immediately (unimplemented for now)
   */
  hipc::FullPtr<GetBlobTask> AsyncGetBlob(
      const hipc::MemContext& mctx,
      chi::u32 tag_id,
      const std::string& blob_name,
      chi::u32 blob_id,
      chi::u64 offset,
      chi::u64 size,
      chi::u32 flags) {
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
        flags);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous reorganize blob - waits for completion (unimplemented for now)
   */
  chi::u32 ReorganizeBlob(const hipc::MemContext& mctx,
                         chi::u32 blob_id,
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
      chi::u32 blob_id,
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
};

}  // namespace wrp_cte::core

#endif  // WRPCTE_CORE_CLIENT_H_