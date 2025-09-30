/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef CHI_hermes_core_H_
#define CHI_hermes_core_H_

#include "chimaera/module_registry/task.h"
#include "hermes_core_tasks.h"

namespace hermes {

/** Create hermes_core requests */
class Client : public ModuleClient {
public:
  /** Default constructor */
  Client() = default;

  /** Destructor */
  ~Client() = default;

  CHI_BEGIN(Create)
  /** Create a task state */
  void Create(const hipc::MemContext &mctx, const DomainQuery &dom_query,
              const DomainQuery &affinity, const std::string &pool_name,
              const CreateContext &ctx = CreateContext()) {
    FullPtr<CreateTask> task =
        AsyncCreate(mctx, dom_query, affinity, pool_name, ctx);
    task->Wait();
    Init(task->ctx_.id_);
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(Create);
  CHI_END(Create)

  CHI_BEGIN(Destroy)
  /** Destroy task state + queue */
  HSHM_INLINE
  void Destroy(const hipc::MemContext &mctx, const DomainQuery &dom_query) {
    CHI_ADMIN->DestroyContainer(mctx, dom_query, pool_id_);
  }
  CHI_TASK_METHODS(Destroy);
  CHI_END(Destroy)

  /**====================================
   * Tag Operations
   * ===================================*/

  CHI_BEGIN(TagUpdateSize)
  /** Update statistics after blob PUT (fire & forget) */
  CHI_TASK_METHODS(TagUpdateSize);
  CHI_END(TagUpdateSize)

  CHI_BEGIN(GetOrCreateTag)
  /** Create a tag or get the ID of existing tag */
  HSHM_INLINE
  TagId GetOrCreateTag(const hipc::MemContext &mctx,
                       const DomainQuery &dom_query,
                       const chi::string &tag_name, bool blob_owner,
                       size_t backend_size, u32 flags,
                       const Context &ctx = Context()) {
    FullPtr<GetOrCreateTagTask> task = AsyncGetOrCreateTag(
        mctx, dom_query, tag_name, blob_owner, backend_size, flags, ctx);
    task->Wait();
    TagId tag_id = task->tag_id_;
    CHI_CLIENT->DelTask(mctx, task);
    return tag_id;
  }
  CHI_TASK_METHODS(GetOrCreateTag);
  CHI_END(GetOrCreateTag)

  CHI_BEGIN(GetTagId)
  /** Get tag ID */
  TagId GetTagId(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                 const chi::string &tag_name) {
    FullPtr<GetTagIdTask> task = AsyncGetTagId(mctx, dom_query, tag_name);
    task->Wait();
    TagId tag_id = task->tag_id_;
    CHI_CLIENT->DelTask(mctx, task);
    return tag_id;
  }
  CHI_TASK_METHODS(GetTagId);
  CHI_END(GetTagId)

  CHI_BEGIN(GetTagName)
  /** Get tag name */
  chi::string GetTagName(const hipc::MemContext &mctx,
                         const DomainQuery &dom_query, const TagId &tag_id) {
    FullPtr<GetTagNameTask> task = AsyncGetTagName(mctx, dom_query, tag_id);
    task->Wait();
    chi::string tag_name(task->tag_name_.str());
    CHI_CLIENT->DelTask(mctx, task);
    return tag_name;
  }
  CHI_TASK_METHODS(GetTagName);
  CHI_END(GetTagName)

  CHI_BEGIN(DestroyTag)
  /** Destroy tag */
  void DestroyTag(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                  const TagId &tag_id) {
    FullPtr<DestroyTagTask> task = AsyncDestroyTag(mctx, dom_query, tag_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(DestroyTag);
  CHI_END(DestroyTag)

  CHI_BEGIN(TagAddBlob)
  /** Add a blob to a tag */
  void TagAddBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                  const TagId &tag_id, const BlobId &blob_id) {
    FullPtr<TagAddBlobTask> task =
        AsyncTagAddBlob(mctx, dom_query, tag_id, blob_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(TagAddBlob);
  CHI_END(TagAddBlob)

  CHI_BEGIN(TagRemoveBlob)
  /** Remove a blob from a tag */
  void TagRemoveBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                     const TagId &tag_id, const BlobId &blob_id) {
    FullPtr<TagRemoveBlobTask> task =
        AsyncTagRemoveBlob(mctx, dom_query, tag_id, blob_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(TagRemoveBlob);
  CHI_END(TagRemoveBlob)

  CHI_BEGIN(TagClearBlobs)
  /** Clear blobs from a tag */
  void TagClearBlobs(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                     const TagId &tag_id) {
    FullPtr<TagClearBlobsTask> task =
        AsyncTagClearBlobs(mctx, dom_query, tag_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(TagClearBlobs);
  CHI_END(TagClearBlobs)

  CHI_BEGIN(TagGetSize)
  /** Get the size of a bucket */
  size_t GetSize(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                 const TagId &tag_id) {
    FullPtr<TagGetSizeTask> task = AsyncTagGetSize(mctx, dom_query, tag_id);
    task->Wait();
    size_t size = task->size_;
    CHI_CLIENT->DelTask(mctx, task);
    return size;
  }
  CHI_TASK_METHODS(TagGetSize);
  CHI_END(TagGetSize)

  CHI_BEGIN(TagGetContainedBlobIds)
  /** Get contained blob ids */
  std::vector<BlobId> TagGetContainedBlobIds(const hipc::MemContext &mctx,
                                             const DomainQuery &dom_query,
                                             const TagId &tag_id) {
    FullPtr<TagGetContainedBlobIdsTask> task =
        AsyncTagGetContainedBlobIds(mctx, dom_query, tag_id);
    task->Wait();
    std::vector<BlobId> blob_ids = task->blob_ids_.vec();
    CHI_CLIENT->DelTask(mctx, task);
    return blob_ids;
  }
  CHI_TASK_METHODS(TagGetContainedBlobIds);
  CHI_END(TagGetContainedBlobIds)

  CHI_BEGIN(TagFlush)
  /** Flush tag */
  void TagFlush(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                const TagId &tag_id) {
    FullPtr<TagFlushTask> task = AsyncTagFlush(mctx, dom_query, tag_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(TagFlush);
  CHI_END(TagFlush)

  /**====================================
   * Blob Operations
   * ===================================*/

  CHI_BEGIN(GetOrCreateBlobId)
  /** Get \a blob_name BLOB from \a bkt_id bucket */
  BlobId GetOrCreateBlob(const hipc::MemContext &mctx,
                         const DomainQuery &dom_query, const TagId &tag_id,
                         const chi::string &blob_name) {
    FullPtr<GetOrCreateBlobIdTask> task =
        AsyncGetOrCreateBlobId(mctx, dom_query, tag_id, blob_name);
    task->Wait();
    BlobId blob_id = task->blob_id_;
    CHI_CLIENT->DelTask(mctx, task);
    return blob_id;
  }
  CHI_TASK_METHODS(GetOrCreateBlobId);
  CHI_END(GetOrCreateBlobId)

  CHI_BEGIN(PutBlob)
  /**
   * Create a blob's metadata
   *
   * @param tag_id id of the bucket
   * @param blob_name semantic blob name
   * @param blob_id the id of the blob
   * @param blob_off the offset of the data placed in existing blob
   * @param blob_size the amount of data being placed
   * @param blob a SHM pointer to the data to place
   * @param score the current score of the blob
   * @param replace whether to replace the blob if it exists
   * @param[OUT] did_create whether the blob was created or not
   * */
  size_t PutBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                 TagId tag_id, const chi::string &blob_name,
                 const BlobId &blob_id, size_t blob_off, size_t blob_size,
                 const hipc::Pointer &blob, float score, u32 task_flags,
                 u32 hermes_flags, Context ctx = Context()) {
    FullPtr<PutBlobTask> task =
        AsyncPutBlob(mctx, dom_query, tag_id, blob_name, blob_id, blob_off,
                     blob_size, blob, score, task_flags, hermes_flags, ctx);
    task->Wait();
    size_t true_size = task->data_size_;
    CHI_CLIENT->DelTask(mctx, task);
    return true_size;
  }
  CHI_TASK_METHODS(PutBlob);
  CHI_END(PutBlob)

  CHI_BEGIN(GetBlob)
  /** Get a blob's data */
  size_t GetBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                 const TagId &tag_id, const BlobId &blob_id, size_t off,
                 ssize_t data_size, hipc::Pointer &data, u32 hermes_flags,
                 const Context &ctx = Context()) {
    FullPtr<GetBlobTask> task =
        AsyncGetBlob(mctx, dom_query, tag_id, chi::string(""), blob_id, off,
                     data_size, data, hermes_flags, ctx);
    task->Wait();
    data = task->data_;
    size_t true_size = task->data_size_;
    CHI_CLIENT->DelTask(mctx, task);
    return true_size;
  }
  CHI_TASK_METHODS(GetBlob);
  CHI_END(GetBlob)

  CHI_BEGIN(AppendBlob)
  /** AppendBlob task */
  void AppendBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                  TagId tag_id, size_t blob_size, const hipc::Pointer &blob,
                  float score, u32 task_flags, u32 hermes_flags,
                  Context ctx = Context()) {
    FullPtr<AppendBlobTask> task =
        AsyncAppendBlob(mctx, dom_query, tag_id, blob_size, blob, score,
                        task_flags, hermes_flags, ctx);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(AppendBlob);
  CHI_END(AppendBlob)

  CHI_BEGIN(ReorganizeBlob)
  /**
   * Reorganize a blob
   *
   * @param blob_id id of the blob being reorganized
   * @param score the new score of the blob
   * @param node_id the node to reorganize the blob to
   * */
  CHI_TASK_METHODS(ReorganizeBlob);
  CHI_END(ReorganizeBlob)

  CHI_BEGIN(ReorganizeNode)
  /** ReorganizeNode task */
  void ReorganizeNode(const hipc::MemContext &mctx,
                      const DomainQuery &dom_query) {
    FullPtr<ReorganizeNodeTask> task = AsyncReorganizeNode(mctx, dom_query);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(ReorganizeNode);
  CHI_END(ReorganizeNode)

  CHI_BEGIN(TagBlob)
  /**
   * Tag a blob
   *
   * @param blob_id id of the blob being tagged
   * @param tag_name tag name
   * */
  void TagBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
               const TagId &tag_id, const BlobId &blob_id, const TagId &tag) {
    FullPtr<TagBlobTask> task =
        AsyncTagBlob(mctx, dom_query, tag_id, blob_id, tag);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(TagBlob);
  CHI_END(TagBlob)

  CHI_BEGIN(BlobHasTag)
  /**
   * Check if blob has a tag
   * */
  bool BlobHasTag(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                  const TagId &tag_id, const BlobId &blob_id,
                  const TagId &tag) {
    FullPtr<BlobHasTagTask> task =
        AsyncBlobHasTag(mctx, dom_query, tag_id, blob_id, tag);
    task->Wait();
    bool has_tag = task->has_tag_;
    CHI_CLIENT->DelTask(mctx, task);
    return has_tag;
  }
  CHI_TASK_METHODS(BlobHasTag);
  CHI_END(BlobHasTag)

  CHI_BEGIN(GetBlobId)
  /**
   * Get \a blob_name BLOB from \a bkt_id bucket
   * */
  BlobId GetBlobId(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                   const TagId &tag_id, const chi::string &blob_name) {
    FullPtr<GetBlobIdTask> task =
        AsyncGetBlobId(mctx, dom_query, tag_id, blob_name);
    task->Wait();
    BlobId blob_id = task->blob_id_;
    CHI_CLIENT->DelTask(mctx, task);
    return blob_id;
  }
  CHI_TASK_METHODS(GetBlobId);
  CHI_END(GetBlobId)

  CHI_BEGIN(GetBlobName)
  /**
   * Get \a blob_name BLOB name from \a blob_id BLOB id
   * */
  std::string GetBlobName(const hipc::MemContext &mctx,
                          const DomainQuery &dom_query, const TagId &tag_id,
                          const BlobId &blob_id) {
    FullPtr<GetBlobNameTask> task =
        AsyncGetBlobName(mctx, dom_query, tag_id, blob_id);
    task->Wait();
    std::string blob_name = task->blob_name_.str();
    CHI_CLIENT->DelTask(mctx, task);
    return blob_name;
  }
  CHI_TASK_METHODS(GetBlobName);
  CHI_END(GetBlobName)

  CHI_BEGIN(GetBlobSize)
  /**
   * Get \a size from \a blob_id BLOB id
   * */
  size_t GetBlobSize(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                     const TagId &tag_id, const chi::string &blob_name,
                     const BlobId &blob_id) {
    FullPtr<GetBlobSizeTask> task =
        AsyncGetBlobSize(mctx, dom_query, tag_id, blob_name, blob_id);
    task->Wait();
    size_t size = task->size_;
    CHI_CLIENT->DelTask(mctx, task);
    return size;
  }
  CHI_TASK_METHODS(GetBlobSize);
  CHI_END(GetBlobSize)

  CHI_BEGIN(GetBlobScore)
  /**
   * Get \a score from \a blob_id BLOB id
   * */
  float GetBlobScore(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                     const TagId &tag_id, const BlobId &blob_id) {
    FullPtr<GetBlobScoreTask> task =
        AsyncGetBlobScore(mctx, dom_query, tag_id, blob_id);
    task->Wait();
    float score = task->score_;
    CHI_CLIENT->DelTask(mctx, task);
    return score;
  }
  CHI_TASK_METHODS(GetBlobScore);
  CHI_END(GetBlobScore)

  CHI_BEGIN(GetBlobBuffers)
  /**
   * Get \a blob_id blob's buffers
   * */
  std::vector<BufferInfo> GetBlobBuffers(const hipc::MemContext &mctx,
                                         const DomainQuery &dom_query,
                                         const TagId &tag_id,
                                         const BlobId &blob_id) {
    FullPtr<GetBlobBuffersTask> task =
        AsyncGetBlobBuffers(mctx, dom_query, tag_id, blob_id);
    task->Wait();
    std::vector<BufferInfo> buffers(task->buffers_.vec());
    CHI_CLIENT->DelTask(mctx, task);
    return buffers;
  }
  CHI_TASK_METHODS(GetBlobBuffers)
  CHI_END(GetBlobBuffers)

  CHI_BEGIN(TruncateBlob)
  /**
   * Truncate a blob to a new size
   * */
  void TruncateBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                    const TagId &tag_id, const BlobId &blob_id,
                    size_t new_size) {
    FullPtr<TruncateBlobTask> task =
        AsyncTruncateBlob(mctx, dom_query, tag_id, blob_id, new_size);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(TruncateBlob);
  CHI_END(TruncateBlob)

  CHI_BEGIN(DestroyBlob)
  /**
   * Destroy \a blob_id blob in \a bkt_id bucket
   * */
  void DestroyBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                   const TagId &tag_id, const BlobId &blob_id,
                   u32 blob_flags = 0) {
    FullPtr<DestroyBlobTask> task =
        AsyncDestroyBlob(mctx, dom_query, tag_id, blob_id, blob_flags);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(DestroyBlob);
  CHI_END(DestroyBlob)

  CHI_BEGIN(FlushBlob)
  /** FlushBlob task */
  void FlushBlob(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                 const BlobId &blob_id) {
    FullPtr<FlushBlobTask> task = AsyncFlushBlob(mctx, dom_query, blob_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(FlushBlob);
  CHI_END(FlushBlob)

  CHI_BEGIN(FlushData)
  /** FlushData task */
  void FlushData(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                 int period_sec = 5) {
    FullPtr<FlushDataTask> task = AsyncFlushData(mctx, dom_query, period_sec);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(FlushData);
  CHI_END(FlushData)

  CHI_BEGIN(PollBlobMetadata)
  /** PollBlobMetadata task */
  std::vector<BlobInfo> PollBlobMetadata(const hipc::MemContext &mctx,
                                         const DomainQuery &dom_query,
                                         const std::string &filter,
                                         int max_count) {
    FullPtr<PollBlobMetadataTask> task =
        AsyncPollBlobMetadata(mctx, dom_query, filter, max_count);
    task->Wait();
    std::vector<BlobInfo> stats = task->GetStats();
    CHI_CLIENT->DelTask(mctx, task);
    return stats;
  }
  CHI_TASK_METHODS(PollBlobMetadata);
  CHI_END(PollBlobMetadata)

  CHI_BEGIN(PollTargetMetadata)
  /** PollTargetMetadata task */
  std::vector<TargetStats> PollTargetMetadata(const hipc::MemContext &mctx,
                                              const DomainQuery &dom_query,
                                              const std::string &filter,
                                              int max_count) {
    FullPtr<PollTargetMetadataTask> task =
        AsyncPollTargetMetadata(mctx, dom_query, filter, max_count);
    task->Wait();
    std::vector<TargetStats> stats = task->GetStats();
    CHI_CLIENT->DelTask(mctx, task);
    return stats;
  }
  CHI_TASK_METHODS(PollTargetMetadata);
  CHI_END(PollTargetMetadata)

  CHI_BEGIN(PollTagMetadata)
  /** PollTagMetadata task */
  std::vector<TagInfo> PollTagMetadata(const hipc::MemContext &mctx,
                                       const DomainQuery &dom_query,
                                       const std::string &filter,
                                       int max_count) {
    FullPtr<PollTagMetadataTask> task =
        AsyncPollTagMetadata(mctx, dom_query, filter, max_count);
    task->Wait();
    std::vector<TagInfo> stats = task->GetStats();
    CHI_CLIENT->DelTask(mctx, task);
    return stats;
  }
  CHI_TASK_METHODS(PollTagMetadata);
  CHI_END(PollTagMetadata)

  CHI_BEGIN(PollAccessPattern)
  /** PollAccessPattern task */
  std::vector<IoStat> PollAccessPattern(const hipc::MemContext &mctx,
                                        const DomainQuery &dom_query,
                                        hshm::min_u64 last_access = 0) {
    FullPtr<PollAccessPatternTask> task =
        AsyncPollAccessPattern(mctx, dom_query, last_access);
    task->Wait();
    std::vector<IoStat> stats = task->io_pattern_.vec();
    CHI_CLIENT->DelTask(mctx, task);
    return stats;
  }
  CHI_TASK_METHODS(PollAccessPattern);
  CHI_END(PollAccessPattern)

  /**
   * ========================================
   * STAGING Tasks
   * ========================================
   * */

  CHI_BEGIN(RegisterStager)
  /** RegisterStager task */
  void RegisterStager(const hipc::MemContext &mctx,
                      const DomainQuery &dom_query,
                      const hermes::BucketId &bkt_id,
                      const chi::string &tag_name, const chi::string &params) {
    FullPtr<RegisterStagerTask> task =
        AsyncRegisterStager(mctx, dom_query, bkt_id, tag_name, params);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(RegisterStager);
  CHI_END(RegisterStager)

  CHI_BEGIN(UnregisterStager)
  /** UnregisterStager task */
  void UnregisterStager(const hipc::MemContext &mctx,
                        const DomainQuery &dom_query, const BucketId &bkt_id) {
    FullPtr<UnregisterStagerTask> task =
        AsyncUnregisterStager(mctx, dom_query, bkt_id);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(UnregisterStager);
  CHI_END(UnregisterStager)

  CHI_BEGIN(StageIn)
  /** StageIn task */
  void StageIn(const hipc::MemContext &mctx, const DomainQuery &dom_query,
               const BucketId &bkt_id, const chi::string &blob_name,
               float score) {
    FullPtr<StageInTask> task =
        AsyncStageIn(mctx, dom_query, bkt_id, blob_name, score);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(StageIn);
  CHI_END(StageIn)

  CHI_BEGIN(StageOut)
  /** StageOut task */
  void StageOut(const hipc::MemContext &mctx, const DomainQuery &dom_query,
                const BucketId &bkt_id, const chi::string &blob_name,
                const hipc::Pointer &data, size_t data_size, u32 task_flags) {
    FullPtr<StageOutTask> task = AsyncStageOut(
        mctx, dom_query, bkt_id, blob_name, data, data_size, task_flags);
    task->Wait();
    CHI_CLIENT->DelTask(mctx, task);
  }
  CHI_TASK_METHODS(StageOut);
  CHI_END(StageOut)

  CHI_AUTOGEN_METHODS
};

} // namespace hermes

#endif // CHI_hermes_core_H_
