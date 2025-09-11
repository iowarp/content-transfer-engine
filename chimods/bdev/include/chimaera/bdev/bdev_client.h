#ifndef BDEV_CLIENT_H_
#define BDEV_CLIENT_H_

#include <chimaera/chimaera.h>
#include <chimaera/bdev/bdev_tasks.h>

namespace chimaera::bdev_extended {

class Client : public chi::ContainerClient {
 public:
  Client() = default;
  explicit Client(const chi::PoolId& pool_id) { Init(pool_id); }

  // ==============================================================================
  // Container Management
  // ==============================================================================

  /**
   * Synchronous container creation (file-based, backward compatible)
   */
  void Create(const hipc::MemContext& mctx, 
              const chi::PoolQuery& pool_query,
              const std::string& file_path, 
              chi::u64 total_size = 0,
              chi::u32 io_depth = 32, 
              chi::u32 alignment = 4096) {
    CreateParams params(hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, CHI_IPC->GetAllocator()),
                       BdevType::kFile, file_path, total_size, io_depth, alignment);
    auto task = AsyncCreate(mctx, pool_query, params);
    task->Wait();
    CHI_IPC->DelTask(task);
  }

  /**
   * Synchronous container creation (with backend type selection)
   */
  void Create(const hipc::MemContext& mctx, 
              const chi::PoolQuery& pool_query,
              BdevType bdev_type, 
              const std::string& file_path = "", 
              chi::u64 total_size = 0,
              chi::u32 io_depth = 32, 
              chi::u32 alignment = 4096) {
    CreateParams params(hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, CHI_IPC->GetAllocator()),
                       bdev_type, file_path, total_size, io_depth, alignment);
    auto task = AsyncCreate(mctx, pool_query, params);
    task->Wait();
    CHI_IPC->DelTask(task);
  }

  /**
   * Asynchronous container creation (file-based, backward compatible)
   */
  hipc::FullPtr<CreateTask> AsyncCreate(
      const hipc::MemContext& mctx,
      const chi::PoolQuery& pool_query,
      const std::string& file_path, 
      chi::u64 total_size = 0,
      chi::u32 io_depth = 32, 
      chi::u32 alignment = 4096) {
    CreateParams params(hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, CHI_IPC->GetAllocator()),
                       BdevType::kFile, file_path, total_size, io_depth, alignment);
    return AsyncCreate(mctx, pool_query, params);
  }

  /**
   * Asynchronous container creation (with backend type selection)
   */
  hipc::FullPtr<CreateTask> AsyncCreate(
      const hipc::MemContext& mctx,
      const chi::PoolQuery& pool_query,
      BdevType bdev_type, 
      const std::string& file_path = "", 
      chi::u64 total_size = 0,
      chi::u32 io_depth = 32, 
      chi::u32 alignment = 4096) {
    CreateParams params(hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, CHI_IPC->GetAllocator()),
                       bdev_type, file_path, total_size, io_depth, alignment);
    return AsyncCreate(mctx, pool_query, params);
  }

  /**
   * Asynchronous container creation with CreateParams
   */
  hipc::FullPtr<CreateTask> AsyncCreate(
      const hipc::MemContext& mctx,
      const chi::PoolQuery& pool_query,
      const CreateParams& params = CreateParams()) {
    auto* ipc_manager = CHI_IPC;
    
    // CRITICAL: CreateTask MUST use admin pool for GetOrCreatePool processing
    auto task = ipc_manager->NewTask<CreateTask>(
        chi::CreateTaskNode(),
        chi::kAdminPoolId,  // Always use admin pool for CreateTask
        pool_query,
        "wrp_cte_bdev_extended",    // ChiMod name
        pool_name_,         // Pool name from base client
        params);            // CreateParams with configuration
    
    // Submit to runtime
    ipc_manager->Enqueue(task);
    return task;
  }

  // ==============================================================================
  // Block Management Operations
  // ==============================================================================

  /**
   * Synchronous block allocation
   */
  Block Allocate(const hipc::MemContext& mctx, chi::u64 size) {
    auto task = AsyncAllocate(mctx, size);
    task->Wait();
    Block result = task->block_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous block allocation
   */
  hipc::FullPtr<AllocateTask> AsyncAllocate(
      const hipc::MemContext& mctx, chi::u64 size) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<AllocateTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        size);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous block free
   */
  chi::u32 Free(const hipc::MemContext& mctx, const Block& block) {
    auto task = AsyncFree(mctx, block);
    task->Wait();
    chi::u32 result = task->result_code_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous block free
   */
  hipc::FullPtr<FreeTask> AsyncFree(
      const hipc::MemContext& mctx, const Block& block) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<FreeTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        block);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  // ==============================================================================
  // I/O Operations
  // ==============================================================================

  /**
   * Synchronous write operation
   */
  chi::u64 Write(const hipc::MemContext& mctx, const Block& block,
                const std::vector<hshm::u8>& data) {
    auto task = AsyncWrite(mctx, block, data);
    task->Wait();
    chi::u64 result = task->bytes_written_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous write operation
   */
  hipc::FullPtr<WriteTask> AsyncWrite(
      const hipc::MemContext& mctx, const Block& block,
      const std::vector<hshm::u8>& data) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<WriteTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        block,
        data);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous read operation
   */
  std::vector<hshm::u8> Read(const hipc::MemContext& mctx, const Block& block) {
    auto task = AsyncRead(mctx, block);
    task->Wait();
    std::vector<hshm::u8> result(task->data_.begin(), task->data_.end());
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous read operation
   */
  hipc::FullPtr<ReadTask> AsyncRead(
      const hipc::MemContext& mctx, const Block& block) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<ReadTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        block);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  // ==============================================================================
  // Performance Monitoring
  // ==============================================================================

  /**
   * Synchronous performance statistics retrieval
   */
  PerfMetrics GetStats(const hipc::MemContext& mctx, chi::u64& remaining_size) {
    auto task = AsyncGetStats(mctx);
    task->Wait();
    PerfMetrics result = task->metrics_;
    remaining_size = task->remaining_size_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous performance statistics retrieval
   */
  hipc::FullPtr<StatTask> AsyncGetStats(
      const hipc::MemContext& mctx) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<StatTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local());
    
    ipc_manager->Enqueue(task);
    return task;
  }

  // ==============================================================================
  // Target Registration APIs
  // ==============================================================================

  /**
   * Synchronous target registration
   * Target name must equal the bdev pool name for this container
   */
  void RegisterTarget(const hipc::MemContext& mctx, const std::string& target_name) {
    auto task = AsyncRegisterTarget(mctx, target_name);
    task->Wait();
    if (task->result_code_ != 0) {
      std::string error_msg = task->error_message_.str();
      CHI_IPC->DelTask(task);
      throw std::runtime_error("RegisterTarget failed: " + error_msg);
    }
    CHI_IPC->DelTask(task);
  }

  /**
   * Asynchronous target registration
   */
  hipc::FullPtr<RegisterTargetTask> AsyncRegisterTarget(
      const hipc::MemContext& mctx, const std::string& target_name) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<RegisterTargetTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        target_name);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target unregistration
   */
  void UnregisterTarget(const hipc::MemContext& mctx, const std::string& target_name) {
    auto task = AsyncUnregisterTarget(mctx, target_name);
    task->Wait();
    if (task->result_code_ != 0) {
      std::string error_msg = task->error_message_.str();
      CHI_IPC->DelTask(task);
      throw std::runtime_error("UnregisterTarget failed: " + error_msg);
    }
    CHI_IPC->DelTask(task);
  }

  /**
   * Asynchronous target unregistration
   */
  hipc::FullPtr<UnregisterTargetTask> AsyncUnregisterTarget(
      const hipc::MemContext& mctx, const std::string& target_name) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<UnregisterTargetTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local(),
        target_name);
    
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target listing
   */
  std::vector<std::string> ListTargets(const hipc::MemContext& mctx) {
    auto task = AsyncListTargets(mctx);
    task->Wait();
    
    std::vector<std::string> result;
    if (task->result_code_ == 0) {
      result.reserve(task->targets_.size());
      for (const auto& target : task->targets_) {
        result.push_back(target.str());
      }
    } else {
      std::string error_msg = task->error_message_.str();
      CHI_IPC->DelTask(task);
      throw std::runtime_error("ListTargets failed: " + error_msg);
    }
    
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target listing
   */
  hipc::FullPtr<ListTargetsTask> AsyncListTargets(
      const hipc::MemContext& mctx) {
    auto* ipc_manager = CHI_IPC;
    auto task = ipc_manager->NewTask<ListTargetsTask>(
        hipc::CtxAllocator<CHI_MAIN_ALLOC_T>(mctx, ipc_manager->GetAllocator()),
        chi::CreateTaskNode(),
        pool_id_,
        chi::PoolQuery::Local());
    
    ipc_manager->Enqueue(task);
    return task;
  }
};

}  // namespace chimaera::bdev_extended

#endif  // BDEV_CLIENT_H_