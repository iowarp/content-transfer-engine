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

#ifndef HRUN_TASKS_HERMES_CONF_INCLUDE_HERMES_CONF_BUCKET_H_
#define HRUN_TASKS_HERMES_CONF_INCLUDE_HERMES_CONF_BUCKET_H_

#include "hermes/config_manager.h"
#include "hermes/hermes_types.h"
#include "hermes_core/hermes_core_client.h"

namespace hermes {

class Bucket {
public:
  hermes::Client mdm_;
  TagId id_;
  chi::string name_;
  Context ctx_;
  hipc::MemContext mctx_;
  bitfield32_t flags_;

public:
  /**====================================
   * Bucket Operations
   * ===================================*/

  /**
   * Get or create \a bkt_name bucket.
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN explicit Bucket(const StringT &bkt_name,
                                 const Context &ctx = Context(),
                                 size_t backend_size = 0, u32 flags = 0) {
    auto *hermes_conf = HERMES_CONF;
    mctx_ = ctx.mctx_;
    ctx_ = ctx;
    mdm_ = hermes_conf->mdm_;
    id_ = mdm_.GetOrCreateTag(mctx_, chi::DomainQuery::GetDynamic(),
                              chi::string(bkt_name), true, backend_size, flags,
                              ctx);
    name_ = bkt_name;
  }

  /**
   * Get an existing bucket.
   * */
  HSHM_CROSS_FUN
  explicit Bucket(TagId tag_id, const Context &ctx = Context()) {
    mctx_ = ctx.mctx_;
    ctx_ = ctx;
    id_ = tag_id;
    mdm_ = HERMES_CONF->mdm_;
  }

  /**
   * Get an existing bucket.
   * */
  HSHM_CROSS_FUN
  explicit Bucket(TagId tag_id, hermes::Client mdm,
                  const Context &ctx = Context()) {
    mctx_ = ctx.mctx_;
    ctx_ = ctx;
    id_ = tag_id;
    mdm_ = mdm;
  }

  /** Default constructor */
  HSHM_CROSS_FUN
  Bucket() = default;

  /** Default copy constructor */
  HSHM_CROSS_FUN
  Bucket(const Bucket &other) = default;

  /** Default copy assign */
  HSHM_CROSS_FUN
  Bucket &operator=(const Bucket &other) = default;

  /** Default move constructor */
  HSHM_CROSS_FUN
  Bucket(Bucket &&other) = default;

  /** Default move assign */
  HSHM_CROSS_FUN
  Bucket &operator=(Bucket &&other) = default;

public:
  /**
   * Get the name of this bucket. Name is cached instead of
   * making an RPC. Not coherent if Rename is called.
   * */
  std::string GetName() const { return name_.str(); }

  /**
   * Get the identifier of this bucket
   * */
  TagId GetId() const { return id_; }

  /**
   * Get the context object of this bucket
   * */
  HSHM_CROSS_FUN
  Context &GetContext() { return ctx_; }

  /**
   * Attach a trait to the bucket
   * */
  void AttachTrait(TraitId trait_id) {
    // TODO(llogan)
  }

  /**
   * Get the current size of the bucket
   * */
  HSHM_CROSS_FUN
  size_t GetSize() {
    return mdm_.GetSize(mctx_, DomainQuery::GetDynamic(), id_);
  }

  /**
   * Set the current size of the bucket
   * */
  HSHM_CROSS_FUN
  void SetSize(size_t new_size) {
    // mdm_.AsyncUpdateSize(id_, new_size, UpdateSizeMode::kCap);
  }

  /**
   * Rename this bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN void Rename(const StringT &new_bkt_name) {
    // mdm_.RenameTag(id_, chi::string(new_bkt_name));
  }

  /**
   * Clears the buckets contents, but doesn't destroy its metadata
   * */
  HSHM_CROSS_FUN
  void Clear() { mdm_.TagClearBlobs(mctx_, DomainQuery::GetDynamic(), id_); }

  /**
   * Destroys this bucket along with all its contents.
   * */
  HSHM_CROSS_FUN
  void Destroy() { mdm_.DestroyTag(mctx_, DomainQuery::GetDynamic(), id_); }

  /**
   * Check if this bucket is valid
   * */
  HSHM_CROSS_FUN
  bool IsNull() { return id_.IsNull(); }

public:
  /**====================================
   * Blob Operations
   * ===================================*/

  /**
   * Get the id of a blob from the blob name
   *
   * @param blob_name the name of the blob
   * @param blob_id (output) the returned blob_id
   * @return
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN BlobId GetBlobId(const StringT &blob_name) {
    return mdm_.GetBlobId(mctx_, DomainQuery::GetDynamic(), id_,
                          chi::string(blob_name));
  }

  /**
   * Get the name of a blob from the blob id
   *
   * @param blob_id the blob_id
   * @param blob_name the name of the blob
   * @return The Status of the operation
   * */
  std::string GetBlobName(const BlobId &blob_id) {
    return mdm_.GetBlobName(mctx_, DomainQuery::GetDynamic(), id_, blob_id);
  }

  /**
   * Get the score of a blob from the blob id
   *
   * @param blob_id the blob_id
   * @return The Status of the operation
   * */
  HSHM_CROSS_FUN
  float GetBlobScore(const BlobId &blob_id) {
    return mdm_.GetBlobScore(mctx_, DomainQuery::GetDynamic(), id_, blob_id);
  }

  /**
   * Label \a blob_id blob with \a tag_name TAG
   * */
  HSHM_CROSS_FUN
  Status TagBlob(BlobId &blob_id, TagId &tag_id) {
    mdm_.TagAddBlob(mctx_, DomainQuery::GetDynamic(), tag_id, blob_id);
    return Status();
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template <bool PARTIAL, bool ASYNC, typename StringT = std::string>
  HSHM_INLINE_CROSS_FUN BlobId ShmBasePut(const StringT &blob_name,
                                          const BlobId &orig_blob_id,
                                          Blob &blob, size_t blob_off,
                                          const Context &ctx) {
    BlobId blob_id = orig_blob_id;
    bitfield32_t task_flags;
    bitfield32_t hermes_flags;
    size_t blob_size = blob.size();
    hipc::Pointer blob_data = blob.shm();
    // Put to shared memory
    chi::string blob_name_buf(blob_name);
    if constexpr (!ASYNC) {
      if (blob_id.IsNull()) {
        hermes_flags.SetBits(HERMES_GET_BLOB_ID);
        task_flags.UnsetBits(TASK_FIRE_AND_FORGET);
      }
    } else {
      blob.Disown();
      task_flags.SetBits(TASK_DATA_OWNER | TASK_FIRE_AND_FORGET);
    }
    if constexpr (!PARTIAL) {
      hermes_flags.SetBits(HERMES_BLOB_REPLACE);
    }
    FullPtr<PutBlobTask> task;
    task = mdm_.AsyncPutBlob(mctx_, chi::DomainQuery::GetDynamic(), id_,
                             blob_name_buf, blob_id, blob_off, blob_size,
                             blob_data, ctx.blob_score_, task_flags.bits_,
                             hermes_flags.bits_, ctx);
    if constexpr (!ASYNC) {
      if (hermes_flags.Any(HERMES_GET_BLOB_ID)) {
        task->Wait();
        blob_id = task->blob_id_;
        CHI_CLIENT->DelTask(mctx_, task);
      }
    }
    return blob_id;
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template <typename T, bool PARTIAL, bool ASYNC,
            typename StringT = std::string>
  HSHM_INLINE_CROSS_FUN BlobId SrlBasePut(const StringT &blob_name,
                                          const BlobId &orig_blob_id,
                                          const T &data, const Context &ctx) {
    std::stringstream ss;
    cereal::BinaryOutputArchive ar(ss);
    ar << data;
    std::string blob_data = ss.str();
    Blob blob(blob_data);
    return ShmBasePut<PARTIAL, ASYNC>(blob_name, orig_blob_id, blob, 0, ctx);
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template <typename T = Blob, typename StringT = std::string>
  HSHM_INLINE_CROSS_FUN BlobId Put(const StringT &blob_name, T &blob,
                                   const Context &ctx = Context()) {
    if constexpr (std::is_same_v<T, Blob>) {
      return ShmBasePut<false, false>(blob_name, BlobId::GetNull(), blob, 0,
                                      ctx);
    } else {
      return SrlBasePut<T, false, false>(blob_name, BlobId::GetNull(), blob,
                                         ctx);
    }
  }

  /**
   * Put \a blob_id Blob into the bucket
   * */
  template <typename T = Blob>
  HSHM_INLINE_CROSS_FUN BlobId Put(const BlobId &blob_id, T &blob,
                                   const Context &ctx = Context()) {
    if constexpr (std::is_same_v<T, Blob>) {
      return ShmBasePut<false, false>(chi::string(""), blob_id, blob, 0, ctx);
    } else {
      return SrlBasePut<T, false, false>(chi::string(""), blob_id, blob, ctx);
    }
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template <typename T = Blob, typename StringT = std::string>
  HSHM_INLINE_CROSS_FUN void AsyncPut(const StringT &blob_name, T &blob,
                                      const Context &ctx = Context()) {
    if constexpr (std::is_same_v<T, Blob>) {
      ShmBasePut<false, true>(blob_name, BlobId::GetNull(), blob, 0, ctx);
    } else {
      SrlBasePut<T, false, true>(blob_name, BlobId::GetNull(), blob, ctx);
    }
  }

  /**
   * Put \a blob_id Blob into the bucket
   * */
  template <typename T>
  HSHM_INLINE_CROSS_FUN void AsyncPut(const BlobId &blob_id, T &blob,
                                      const Context &ctx = Context()) {
    if constexpr (std::is_same_v<T, Blob>) {
      ShmBasePut<false, true>(chi::string(""), blob_id, blob, 0, ctx);
    } else {
      SrlBasePut<T, false, true>(chi::string(""), blob_id, blob, ctx);
    }
  }

  /**
   * PartialPut \a blob_name Blob into the bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN BlobId PartialPut(const StringT &blob_name, Blob &blob,
                                   size_t blob_off,
                                   const Context &ctx = Context()) {
    return ShmBasePut<true, false>(blob_name, BlobId::GetNull(), blob, blob_off,
                                   ctx);
  }

  /**
   * PartialPut \a blob_id Blob into the bucket
   * */
  HSHM_CROSS_FUN
  BlobId PartialPut(const BlobId &blob_id, Blob &blob, size_t blob_off,
                    const Context &ctx = Context()) {
    return ShmBasePut<true, false>(chi::string(""), blob_id, blob, blob_off,
                                   ctx);
  }

  /**
   * AsyncPartialPut \a blob_name Blob into the bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN void AsyncPartialPut(const StringT &blob_name, Blob &blob,
                                      size_t blob_off,
                                      const Context &ctx = Context()) {
    ShmBasePut<true, true>(blob_name, BlobId::GetNull(), blob, blob_off, ctx);
  }

  /**
   * AsyncPartialPut \a blob_id Blob into the bucket
   * */
  HSHM_CROSS_FUN
  void AsyncPartialPut(const BlobId &blob_id, Blob &blob, size_t blob_off,
                       const Context &ctx = Context()) {
    ShmBasePut<true, true>(chi::string(""), blob_id, blob, blob_off, ctx);
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template <bool ASYNC>
  HSHM_INLINE_CROSS_FUN void ShmBaseAppend(Blob &blob, const Context &ctx) {
    bitfield32_t task_flags;
    bitfield32_t hermes_flags;
    size_t blob_size = blob.size();
    hipc::Pointer blob_data = blob.shm();
    // Put to shared memory
    if constexpr (ASYNC) {
      blob.Disown();
      task_flags.SetBits(TASK_DATA_OWNER | TASK_FIRE_AND_FORGET);
    }
    FullPtr<AppendBlobTask> task;
    task = mdm_.AsyncAppendBlob(mctx_, DomainQuery::GetDynamic(), id_,
                                blob.size(), blob_data, ctx.blob_score_,
                                task_flags.bits_, hermes_flags.bits_, ctx);
    if constexpr (!ASYNC) {
      task->Wait();
      CHI_CLIENT->DelTask(mctx_, task);
    }
  }

  /**
   * Append \a blob_name Blob into the bucket (fully asynchronous)
   * */
  HSHM_CROSS_FUN void Append(Blob &blob, size_t page_size,
                             const Context &ctx = Context()) {
    ShmBaseAppend<false>(blob, ctx);
  }

  /**
   * Append \a blob_name Blob into the bucket (fully asynchronous)
   * */
  HSHM_CROSS_FUN
  void AsyncAppend(Blob &blob, size_t page_size,
                   const Context &ctx = Context()) {
    ShmBaseAppend<true>(blob, ctx);
  }

  /**
   * Reorganize a blob to a new score or node
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN void ReorganizeBlob(const StringT &name, float score,
                                     const Context &ctx = Context()) {
    mdm_.AsyncReorganizeBlob(mctx_, DomainQuery::GetDynamic(), id_,
                             chi::string(name), BlobId::GetNull(), score, true,
                             ctx);
  }

  /**
   * Reorganize a blob to a new score or node
   * */
  HSHM_CROSS_FUN
  void ReorganizeBlob(const BlobId &blob_id, float score,
                      const Context &ctx = Context()) {
    mdm_.AsyncReorganizeBlob(mctx_, DomainQuery::GetDynamic(), id_,
                             chi::string(""), blob_id, score, true, ctx);
  }

  /**
   * Reorganize a blob to a new score or node
   *
   * @depricated
   * */
  HSHM_CROSS_FUN
  void ReorganizeBlob(const BlobId &blob_id, float score, u32 node_id,
                      Context &ctx) {
    ctx.node_id_ = node_id;
    mdm_.AsyncReorganizeBlob(mctx_, DomainQuery::GetDynamic(), id_,
                             chi::string(""), blob_id, score, true, ctx);
  }

  /**
   * Get the current size of the blob in the bucket
   * */
  HSHM_CROSS_FUN
  size_t GetBlobSize(const BlobId &blob_id) {
    return mdm_.GetBlobSize(mctx_, DomainQuery::GetDynamic(), id_,
                            chi::string(""), blob_id);
  }

  /**
   * Get the current size of the blob in the bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN size_t GetBlobSize(const StringT &name) {
    return mdm_.GetBlobSize(mctx_, DomainQuery::GetDynamic(), id_,
                            chi::string(name), BlobId::GetNull());
  }

  /**
   * Get the current size of the blob in the bucket
   */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN size_t GetBlobSize(const StringT &name,
                                    const BlobId &blob_id) {
    if (!name.empty()) {
      return GetBlobSize(name);
    } else {
      return GetBlobSize(blob_id);
    }
  }

  /**
   * Get \a blob_id Blob from the bucket (async)
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN FullPtr<GetBlobTask> HSHM_INLINE
  ShmAsyncBaseGet(const StringT &blob_name, const BlobId &blob_id, Blob &blob,
                  size_t blob_off, const Context &ctx = Context()) {
    bitfield32_t hermes_flags;
    // Get the blob ID
    if (blob_id.IsNull()) {
      hermes_flags.SetBits(HERMES_GET_BLOB_ID);
    }
    // Get blob size
    if (blob.data_.shm_.IsNull()) {
      size_t size = GetBlobSize(blob_name, blob_id);
      blob.resize(size);
    }
    // Get from shared memory
    FullPtr<GetBlobTask> task;
    task = mdm_.AsyncGetBlob(mctx_, chi::DomainQuery::GetDynamic(), id_,
                             chi::string(blob_name), blob_id, blob_off,
                             blob.size(), blob.shm(), hermes_flags.bits_, ctx);
    return task;
  }

  /**
   * Get \a blob_id Blob from the bucket (sync)
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN BlobId ShmBaseGet(const StringT &blob_name,
                                   const BlobId &orig_blob_id, Blob &blob,
                                   size_t blob_off,
                                   const Context &ctx = Context()) {
    HILOG(kDebug, "Getting blob of size {}", blob.size());
    BlobId blob_id;
    FullPtr<GetBlobTask> task;
    task = ShmAsyncBaseGet(blob_name, orig_blob_id, blob, blob_off, ctx);
    task->Wait();
    blob_id = task->blob_id_;
    CHI_CLIENT->DelTask(mctx_, task);
    return blob_id;
  }

  /**
   * Get \a blob_id Blob from the bucket (sync)
   * */
  template <typename T, typename StringT = std::string>
  HSHM_CROSS_FUN BlobId SrlBaseGet(const StringT &blob_name,
                                   const BlobId &orig_blob_id, T &data,
                                   const Context &ctx = Context()) {
    Blob blob;
    BlobId blob_id = ShmBaseGet(blob_name, orig_blob_id, blob, 0, ctx);
    if (blob.size() == 0) {
      return BlobId::GetNull();
    }
    std::stringstream ss(std::string(blob.data(), blob.size()));
    cereal::BinaryInputArchive ar(ss);
    ar >> data;
    return blob_id;
  }

  /**
   * Get \a blob_id Blob from the bucket
   * */
  template <typename T, typename StringT = std::string>
  HSHM_CROSS_FUN BlobId Get(const StringT &blob_name, T &blob,
                            const Context &ctx = Context()) {
    if constexpr (std::is_same_v<T, Blob>) {
      return ShmBaseGet(blob_name, BlobId::GetNull(), blob, 0, ctx);
    } else {
      return SrlBaseGet<T>(blob_name, BlobId::GetNull(), blob, ctx);
    }
  }

  /**
   * Get \a blob_id Blob from the bucket
   * */
  template <typename T>
  HSHM_CROSS_FUN BlobId Get(const BlobId &blob_id, T &blob,
                            const Context &ctx = Context()) {
    if constexpr (std::is_same_v<T, Blob>) {
      return ShmBaseGet(chi::string(""), blob_id, blob, 0, ctx);
    } else {
      return SrlBaseGet<T>(chi::string(""), blob_id, blob, ctx);
    }
  }

  /**
   * AsyncGet \a blob_name Blob from the bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN FullPtr<GetBlobTask> AsyncGet(const StringT &blob_name,
                                               Blob &blob,
                                               const Context &ctx = Context()) {
    return ShmAsyncBaseGet(blob_name, BlobId::GetNull(), blob, 0, ctx);
  }

  /**
   * AsyncGet \a blob_id Blob from the bucket
   * */
  HSHM_CROSS_FUN
  FullPtr<GetBlobTask> AsyncGet(const BlobId &blob_id, Blob &blob,
                                const Context &ctx = Context()) {
    return ShmAsyncBaseGet(chi::string(""), blob_id, blob, 0, ctx);
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN BlobId PartialGet(const StringT &blob_name, Blob &blob,
                                   size_t blob_off,
                                   const Context &ctx = Context()) {
    return ShmBaseGet(blob_name, BlobId::GetNull(), blob, blob_off, ctx);
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  HSHM_CROSS_FUN
  BlobId PartialGet(const BlobId &blob_id, Blob &blob, size_t blob_off,
                    const Context &ctx = Context()) {
    return ShmBaseGet(chi::string(""), blob_id, blob, blob_off, ctx);
  }

  /**
   * AsyncGet \a blob_name Blob from the bucket
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN FullPtr<GetBlobTask>
  AsyncPartialGet(const StringT &blob_name, Blob &blob, size_t blob_off,
                  const Context &ctx = Context()) {
    return ShmAsyncBaseGet(blob_name, BlobId::GetNull(), blob, blob_off, ctx);
  }

  /**
   * AsyncGet \a blob_id Blob from the bucket
   * */
  HSHM_CROSS_FUN
  FullPtr<GetBlobTask> AsyncPartialGet(const BlobId &blob_id, Blob &blob,
                                       size_t blob_off,
                                       const Context &ctx = Context()) {
    return ShmAsyncBaseGet(chi::string(""), blob_id, blob, blob_off, ctx);
  }

  /**
   * Determine if the bucket contains \a blob_id BLOB
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN bool ContainsBlob(const StringT &blob_name) {
    BlobId new_blob_id = mdm_.GetBlobId(mctx_, DomainQuery::GetDynamic(), id_,
                                        chi::string(blob_name));
    return !new_blob_id.IsNull();
  }

  /**
   * Rename \a blob_id blob to \a new_blob_name new name
   * */
  template <typename StringT = std::string>
  HSHM_CROSS_FUN void RenameBlob(const BlobId &blob_id,
                                 const StringT &new_blob_name,
                                 const Context &ctx = Context()) {
    // mdm_.RenameBlob(id_, blob_id, chi::string(new_blob_name));
  }

  /**
   * Delete \a blob_id blob
   * */
  HSHM_CROSS_FUN
  void DestroyBlob(const BlobId &blob_id, const Context &ctx = Context()) {
    mdm_.DestroyBlob(mctx_, DomainQuery::GetDynamic(), id_, blob_id);
  }

  /**
   * Get the set of blob IDs contained in the bucket
   * */
  std::vector<BlobId> GetContainedBlobIds() {
    return mdm_.TagGetContainedBlobIds(mctx_, DomainQuery::GetDynamic(), id_);
  }

  /** Flush the bucket */
  HSHM_CROSS_FUN
  void Flush() { mdm_.TagFlush(mctx_, DomainQuery::GetDynamic(), id_); }
};

} // namespace hermes

#endif // HRUN_TASKS_HERMES_CONF_INCLUDE_HERMES_CONF_BUCKET_H_
