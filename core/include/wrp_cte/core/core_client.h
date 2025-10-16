#ifndef WRPCTE_CORE_CLIENT_H_
#define WRPCTE_CORE_CLIENT_H_

#include <chimaera/chimaera.h>
#include <hermes_shm/util/singleton.h>
#include <wrp_cte/core/core_tasks.h>

namespace wrp_cte::core {

class Client : public chi::ContainerClient {
public:
  Client() = default;
  explicit Client(const chi::PoolId &pool_id) { Init(pool_id); }

  /**
   * Synchronous container creation - waits for completion
   */
  void Create(const hipc::MemContext &mctx, const chi::PoolQuery &pool_query,
              const CreateParams &params = CreateParams()) {
    auto task = AsyncCreate(mctx, pool_query, params);
    task->Wait();

    // Check if CreateTask succeeded and update client's pool_id_ to the actual
    // pool created/found This is required because CreateTask is a
    // GetOrCreatePoolTask that may return a different pool ID than what was
    // requested if the pool already existed
    if (task->return_code_ == 0) {
      pool_id_ = task->new_pool_id_;
    }

    CHI_IPC->DelTask(task);
  }

  /**
   * Asynchronous container creation - returns immediately
   */
  hipc::FullPtr<CreateTask>
  AsyncCreate(const hipc::MemContext &mctx, const chi::PoolQuery &pool_query,
              const CreateParams &params = CreateParams()) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    // CRITICAL: CreateTask MUST use admin pool for GetOrCreatePool processing
    auto task = ipc_manager->NewTask<CreateTask>(
        chi::CreateTaskId(),
        chi::kAdminPoolId, // Always use admin pool for CreateTask
        pool_query,
        "wrp_cte_core", // ChiMod name
        "wrp_cte_core", // Pool name as string
        pool_id_,       // Target pool ID
        params);        // CreateParams with configuration

    // Submit to runtime
    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target registration - waits for completion
   */
  chi::u32 RegisterTarget(const hipc::MemContext &mctx,
                          const std::string &target_name,
                          chimaera::bdev::BdevType bdev_type,
                          chi::u64 total_size) {
    auto task = AsyncRegisterTarget(mctx, target_name, bdev_type, total_size);
    task->Wait();
    chi::u32 result = task->return_code_.load();
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target registration - returns immediately
   */
  hipc::FullPtr<RegisterTargetTask>
  AsyncRegisterTarget(const hipc::MemContext &mctx,
                      const std::string &target_name,
                      chimaera::bdev::BdevType bdev_type, chi::u64 total_size) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<RegisterTargetTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), target_name,
        bdev_type, total_size);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target unregistration - waits for completion
   */
  chi::u32 UnregisterTarget(const hipc::MemContext &mctx,
                            const std::string &target_name) {
    auto task = AsyncUnregisterTarget(mctx, target_name);
    task->Wait();
    chi::u32 result = task->return_code_.load();
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target unregistration - returns immediately
   */
  hipc::FullPtr<UnregisterTargetTask>
  AsyncUnregisterTarget(const hipc::MemContext &mctx,
                        const std::string &target_name) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<UnregisterTargetTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), target_name);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target listing - waits for completion
   */
  std::vector<std::string> ListTargets(const hipc::MemContext &mctx) {
    auto task = AsyncListTargets(mctx);
    task->Wait();

    // Convert HSHM vector to standard vector for client use
    std::vector<std::string> result;
    result.reserve(task->target_names_.size());
    for (const auto &target_name : task->target_names_) {
      result.push_back(target_name.str());
    }

    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target listing - returns immediately
   */
  hipc::FullPtr<ListTargetsTask>
  AsyncListTargets(const hipc::MemContext &mctx) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<ListTargetsTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local());

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous target stats update - waits for completion
   */
  chi::u32 StatTargets(const hipc::MemContext &mctx) {
    auto task = AsyncStatTargets(mctx);
    task->Wait();
    chi::u32 result = task->return_code_.load();
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous target stats update - returns immediately
   */
  hipc::FullPtr<StatTargetsTask>
  AsyncStatTargets(const hipc::MemContext &mctx) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<StatTargetsTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local());

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get or create tag - waits for completion
   */
  TagId GetOrCreateTag(const hipc::MemContext &mctx,
                       const std::string &tag_name,
                       const TagId &tag_id = TagId::GetNull()) {
    auto task = AsyncGetOrCreateTag(mctx, tag_name, tag_id);
    task->Wait();

    TagId result = task->tag_id_;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get or create tag - returns immediately
   */
  hipc::FullPtr<GetOrCreateTagTask<CreateParams>>
  AsyncGetOrCreateTag(const hipc::MemContext &mctx, const std::string &tag_name,
                      const TagId &tag_id = TagId::GetNull()) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<GetOrCreateTagTask<CreateParams>>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_name,
        tag_id);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous put blob - waits for completion
   */
  bool PutBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id,
               chi::u64 offset, chi::u64 size, hipc::Pointer blob_data,
               float score, chi::u32 flags) {
    auto task = AsyncPutBlob(mctx, tag_id, blob_name, blob_id, offset, size,
                             blob_data, score, flags);
    task->Wait();
    bool result = (task->return_code_.load() == 0);
    if (!result) {
      HELOG(kError, "PutBlob failed: {}", task->return_code_.load());
    }
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous put blob - returns immediately (unimplemented for now)
   */
  hipc::FullPtr<PutBlobTask>
  AsyncPutBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id,
               chi::u64 offset, chi::u64 size, hipc::Pointer blob_data,
               float score, chi::u32 flags) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<PutBlobTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id,
        blob_name, blob_id, offset, size, blob_data, score, flags);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get blob - waits for completion
   */
  bool GetBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id,
               chi::u64 offset, chi::u64 size, chi::u32 flags,
               hipc::Pointer blob_data) {
    auto task = AsyncGetBlob(mctx, tag_id, blob_name, blob_id, offset, size,
                             flags, blob_data);
    task->Wait();
    bool result = (task->return_code_.load() == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get blob - returns immediately
   */
  hipc::FullPtr<GetBlobTask>
  AsyncGetBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id,
               chi::u64 offset, chi::u64 size, chi::u32 flags,
               hipc::Pointer blob_data) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<GetBlobTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id,
        blob_name, blob_id, offset, size, flags, blob_data);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous reorganize blobs - waits for completion
   */
  chi::u32 ReorganizeBlobs(const hipc::MemContext &mctx, const TagId &tag_id,
                           const std::vector<std::string> &blob_names,
                           const std::vector<float> &new_scores) {
    auto task = AsyncReorganizeBlobs(mctx, tag_id, blob_names, new_scores);
    task->Wait();
    chi::u32 result = task->return_code_.load();
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous reorganize blobs - returns immediately
   */
  hipc::FullPtr<ReorganizeBlobsTask>
  AsyncReorganizeBlobs(const hipc::MemContext &mctx, const TagId &tag_id,
                       const std::vector<std::string> &blob_names,
                       const std::vector<float> &new_scores) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<ReorganizeBlobsTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id,
        blob_names, new_scores);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous delete blob - waits for completion
   */
  bool DelBlob(const hipc::MemContext &mctx, const TagId &tag_id,
               const std::string &blob_name, const BlobId &blob_id) {
    auto task = AsyncDelBlob(mctx, tag_id, blob_name, blob_id);
    task->Wait();
    bool result = (task->return_code_.load() == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous delete blob - returns immediately
   */
  hipc::FullPtr<DelBlobTask> AsyncDelBlob(const hipc::MemContext &mctx,
                                          const TagId &tag_id,
                                          const std::string &blob_name,
                                          const BlobId &blob_id) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<DelBlobTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id,
        blob_name, blob_id);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous delete tag by tag ID - waits for completion
   */
  bool DelTag(const hipc::MemContext &mctx, const TagId &tag_id) {
    auto task = AsyncDelTag(mctx, tag_id);
    task->Wait();
    bool result = (task->return_code_.load() == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Synchronous delete tag by tag name - waits for completion
   */
  bool DelTag(const hipc::MemContext &mctx, const std::string &tag_name) {
    auto task = AsyncDelTag(mctx, tag_name);
    task->Wait();
    bool result = (task->return_code_.load() == 0);
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous delete tag by tag ID - returns immediately
   */
  hipc::FullPtr<DelTagTask> AsyncDelTag(const hipc::MemContext &mctx,
                                        const TagId &tag_id) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<DelTagTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Asynchronous delete tag by tag name - returns immediately
   */
  hipc::FullPtr<DelTagTask> AsyncDelTag(const hipc::MemContext &mctx,
                                        const std::string &tag_name) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<DelTagTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_name);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get tag size - waits for completion
   */
  size_t GetTagSize(const hipc::MemContext &mctx, const TagId &tag_id) {
    auto task = AsyncGetTagSize(mctx, tag_id);
    task->Wait();
    size_t result = (task->return_code_.load() == 0) ? task->tag_size_ : 0;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get tag size - returns immediately
   */
  hipc::FullPtr<GetTagSizeTask> AsyncGetTagSize(const hipc::MemContext &mctx,
                                                const TagId &tag_id) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<GetTagSizeTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous poll telemetry log - waits for completion
   */
  std::vector<CteTelemetry>
  PollTelemetryLog(const hipc::MemContext &mctx,
                   std::uint64_t minimum_logical_time) {
    auto task = AsyncPollTelemetryLog(mctx, minimum_logical_time);
    task->Wait();

    // Convert HSHM vector to standard vector for client use
    std::vector<CteTelemetry> result;
    result.reserve(task->entries_.size());
    for (const auto &entry : task->entries_) {
      result.push_back(entry);
    }

    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous poll telemetry log - returns immediately
   */
  hipc::FullPtr<PollTelemetryLogTask>
  AsyncPollTelemetryLog(const hipc::MemContext &mctx,
                        std::uint64_t minimum_logical_time) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<PollTelemetryLogTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(),
        minimum_logical_time);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get blob score - waits for completion
   */
  float GetBlobScore(const hipc::MemContext &mctx, const TagId &tag_id,
                     const std::string &blob_name,
                     const BlobId &blob_id = BlobId::GetNull()) {
    auto task = AsyncGetBlobScore(mctx, tag_id, blob_name, blob_id);
    task->Wait();
    float result = (task->return_code_.load() == 0) ? task->score_ : 0.0f;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get blob score - returns immediately
   */
  hipc::FullPtr<GetBlobScoreTask>
  AsyncGetBlobScore(const hipc::MemContext &mctx, const TagId &tag_id,
                    const std::string &blob_name,
                    const BlobId &blob_id = BlobId::GetNull()) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<GetBlobScoreTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id,
        blob_name, blob_id);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get blob size - waits for completion
   */
  chi::u64 GetBlobSize(const hipc::MemContext &mctx, const TagId &tag_id,
                       const std::string &blob_name,
                       const BlobId &blob_id = BlobId::GetNull()) {
    auto task = AsyncGetBlobSize(mctx, tag_id, blob_name, blob_id);
    task->Wait();
    chi::u64 result = (task->return_code_.load() == 0) ? task->size_ : 0;
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get blob size - returns immediately
   */
  hipc::FullPtr<GetBlobSizeTask>
  AsyncGetBlobSize(const hipc::MemContext &mctx, const TagId &tag_id,
                   const std::string &blob_name,
                   const BlobId &blob_id = BlobId::GetNull()) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<GetBlobSizeTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id,
        blob_name, blob_id);

    ipc_manager->Enqueue(task);
    return task;
  }

  /**
   * Synchronous get contained blobs - waits for completion
   */
  std::vector<std::string> GetContainedBlobs(const hipc::MemContext &mctx,
                                             const TagId &tag_id) {
    auto task = AsyncGetContainedBlobs(mctx, tag_id);
    task->Wait();
    std::vector<std::string> result;
    if (task->return_code_.load() == 0) {
      for (const auto &blob_name : task->blob_names_) {
        result.emplace_back(blob_name.str());
      }
    }
    CHI_IPC->DelTask(task);
    return result;
  }

  /**
   * Asynchronous get contained blobs - returns immediately
   */
  hipc::FullPtr<GetContainedBlobsTask>
  AsyncGetContainedBlobs(const hipc::MemContext &mctx, const TagId &tag_id) {
    (void)mctx; // Suppress unused parameter warning
    auto *ipc_manager = CHI_IPC;

    auto task = ipc_manager->NewTask<GetContainedBlobsTask>(
        chi::CreateTaskId(), pool_id_, chi::PoolQuery::Local(), tag_id);

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
bool WRP_CTE_CLIENT_INIT(const std::string &config_path = "");

/**
 * Tag wrapper class - provides convenient API for tag operations
 */
class Tag {
private:
  TagId tag_id_;
  std::string tag_name_;

public:
  /**
   * Constructor - Call the WRP_CTE client GetOrCreateTag function
   * @param tag_name Tag name to get or create
   */
  explicit Tag(const std::string &tag_name);

  /**
   * Constructor - Does not call WRP_CTE client function, just sets the TagId
   * variable
   * @param tag_id Tag ID to use directly
   */
  explicit Tag(const TagId &tag_id);

  /**
   * PutBlob - Allocates a SHM pointer and then calls PutBlob (SHM)
   * @param blob_name Name of the blob
   * @param data Raw data pointer
   * @param data_size Size of data
   * @param off Offset within blob (default 0)
   */
  void PutBlob(const std::string &blob_name, const char *data, size_t data_size,
               size_t off = 0);

  /**
   * PutBlob (SHM) - Direct shared memory version
   * @param blob_name Name of the blob
   * @param data Shared memory pointer to data
   * @param data_size Size of data
   * @param off Offset within blob (default 0)
   * @param score Blob score for placement decisions (default 1.0)
   */
  void PutBlob(const std::string &blob_name, const hipc::Pointer &data,
               size_t data_size, size_t off = 0, float score = 1.0f);

  /**
   * Asynchronous PutBlob (SHM) - Caller must manage shared memory lifecycle
   * @param blob_name Name of the blob
   * @param data Shared memory pointer to data (must remain valid until task
   * completes)
   * @param data_size Size of data
   * @param off Offset within blob (default 0)
   * @param score Blob score for placement decisions (default 1.0)
   * @return Task pointer for async operation
   * @note For raw data, caller must allocate shared memory using
   * CHI_IPC->AllocateBuffer<void>() and keep the FullPtr alive until the async
   * task completes
   */
  hipc::FullPtr<PutBlobTask> AsyncPutBlob(const std::string &blob_name,
                                          const hipc::Pointer &data,
                                          size_t data_size, size_t off = 0,
                                          float score = 1.0f);

  /**
   * GetBlob - Allocates shared memory, retrieves blob data, copies to output
   * buffer
   * @param blob_name Name of the blob to retrieve
   * @param data Output buffer to copy blob data into (must be pre-allocated by
   * caller)
   * @param data_size Size of data to retrieve (must be > 0)
   * @param off Offset within blob (default 0)
   * @note Automatically handles shared memory allocation/deallocation
   */
  void GetBlob(const std::string &blob_name, char *data, size_t data_size,
               size_t off = 0);

  /**
   * GetBlob (SHM) - Retrieves blob data into pre-allocated shared memory buffer
   * @param blob_name Name of the blob to retrieve
   * @param data Pre-allocated shared memory pointer for output data (must not
   * be null)
   * @param data_size Size of data to retrieve (must be > 0)
   * @param off Offset within blob (default 0)
   * @note Caller must pre-allocate shared memory using
   * CHI_IPC->AllocateBuffer<void>(data_size)
   */
  void GetBlob(const std::string &blob_name, hipc::Pointer data,
               size_t data_size, size_t off = 0);

  /**
   * Get blob score
   * @param blob_name Name of the blob
   * @return Blob score (0.0-1.0)
   */
  float GetBlobScore(const std::string &blob_name);

  /**
   * Get blob size
   * @param blob_name Name of the blob
   * @return Blob size in bytes
   */
  chi::u64 GetBlobSize(const std::string &blob_name);

  /**
   * Get all blob names contained in this tag
   * @return Vector of blob names in this tag
   */
  std::vector<std::string> GetContainedBlobs();

  /**
   * Get the TagId for this tag
   * @return TagId of this tag
   */
  const TagId &GetTagId() const { return tag_id_; }
};

} // namespace wrp_cte::core

// Global singleton macros for easy access (return pointers, not references)
#define WRP_CTE_CLIENT                                                         \
  (&(*HSHM_GET_GLOBAL_PTR_VAR(wrp_cte::core::Client,                           \
                              wrp_cte::core::g_cte_client)))
#define WRP_CTE_CONFIG                                                         \
  (&(*HSHM_GET_GLOBAL_PTR_VAR(wrp_cte::core::Config,                           \
                              wrp_cte::core::g_cte_config)))

#endif // WRPCTE_CORE_CLIENT_H_