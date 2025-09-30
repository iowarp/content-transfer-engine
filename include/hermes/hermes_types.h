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

#ifndef HRUN_TASKS_HERMES_INCLUDE_HERMES_HERMES_TYPES_H_
#define HRUN_TASKS_HERMES_INCLUDE_HERMES_HERMES_TYPES_H_

#include "bdev/bdev_client.h"
#include "chimaera/chimaera_types.h"
#include "status.h"
#include "statuses.h"

namespace hapi = hermes;

namespace hermes {

CHI_NAMESPACE_INIT

using chi::UniqueId;
using hshm::bitfield32_t;

/** Queue id */
using chi::QueueId;

/** Unique blob id */
typedef UniqueId<100> BlobId;

/** Unique bucket id */
typedef UniqueId<101> BucketId;

/** Represents a tag */
typedef UniqueId<102> TagId;

/** Represents a storage target */
typedef PoolId TargetId;

/** Represents a target */
struct TargetInfo {
  TargetId id_;
  chi::bdev::Client client_;
  DomainQuery dom_query_;
  FullPtr<chi::bdev::PollStatsTask> poll_stats_;
  chi::BdevStats *stats_;
  float score_ = 0; // TODO(llogan): Calculate score

  size_t GetRemCap() { return stats_->free_; }
};

/** Basic target statistics summary */
struct TargetStats {
  TargetId tgt_id_;
  chi::NodeId node_id_;
  ssize_t rem_cap_;
  ssize_t max_cap_;
  float bandwidth_;
  float latency_;
  float score_;

  template <typename Ar> void serialize(Ar &ar) {
    ar(tgt_id_, node_id_, rem_cap_, max_cap_, bandwidth_, latency_, score_);
  }
};

/** Represents a trait */
typedef PoolId TraitId;

/** Different categories of traits */
enum class TraitType { kStagingTrait, kProducerOpTrait };

/** Represents a blob */
class Blob {
public:
  hipc::FullPtr<char> data_ = hipc::FullPtr<char>::GetNull();
  size_t size_ = 0;
  size_t max_size_ = 0;
  bool owned_ = true;

public:
  /** Default constructor */
  HSHM_INLINE_CROSS_FUN
  Blob() = default;

  /** Size constructor */
  HSHM_INLINE_CROSS_FUN
  Blob(size_t size) { resize(size); }

  /** Copy string */
  Blob(const std::string &data) { Copy(data.data(), data.size()); }

  /** Copy vector */
  template <typename T> HSHM_INLINE_CROSS_FUN Blob(const chi::vector<T> &data) {
    Copy(data.data(), data.size() * sizeof(T));
  }

  /** Copy char *, size */
  HSHM_INLINE_CROSS_FUN
  Blob(const char *data, size_t size) {
    FullPtr<char> data_full(data);
    Copy(data_full.ptr_, size);
  }

  /** Copy shm pointer, size */
  HSHM_INLINE_CROSS_FUN
  Blob(const hipc::Pointer &data, size_t size) {
    FullPtr<char> data_full(data);
    Copy(data_full.ptr_, size);
  }

  /** Wrap shm pointer */
  HSHM_INLINE_CROSS_FUN
  Blob(const hipc::Pointer &data, size_t size, bool owned) {
    FullPtr<char> data_full(data);
    data_ = data_full;
    size_ = size;
    owned_ = false;
  }

  /** Copy assignment */
  HSHM_INLINE_CROSS_FUN
  Blob &operator=(const Blob &other) {
    if (this != &other) {
      Copy(other.data(), other.size());
    }
    return *this;
  }

  /** Copy constructor */
  HSHM_INLINE_CROSS_FUN
  Blob(const Blob &other) { Copy(other.data(), other.size()); }

  /** Copy */
  HSHM_CROSS_FUN
  void Copy(const char *other, size_t other_size) {
    if (!data_.IsNull()) {
      HSHM_THROW_STD_ERROR("Copy to preallocated blob not supported");
    }
    resize(other_size);
    memcpy(data_.ptr_, other, other_size);
  }

  /** Move constructor */
  HSHM_CROSS_FUN
  Blob(Blob &&other) {
    data_ = other.data_;
    size_ = other.size_;
    max_size_ = other.max_size_;
    other.Disown();
  }

  /** Move assignment */
  HSHM_CROSS_FUN
  Blob &operator=(Blob &&other) {
    if (this != &other) {
      data_ = other.data_;
      size_ = other.size_;
      max_size_ = other.max_size_;
      other.Disown();
    }
    return *this;
  }

  /** Disown */
  HSHM_INLINE_CROSS_FUN
  void Disown() { owned_ = false; }

  /** Destructor */
  HSHM_CROSS_FUN
  ~Blob() {
    if (!data_.IsNull() && owned_) {
      CHI_CLIENT->FreeBuffer(HSHM_MCTX, data_);
    }
  }

  /** Resize */
  HSHM_CROSS_FUN
  void resize(size_t new_size) {
    reserve(new_size);
    size_ = new_size;
  }

  /** Reserve */
  HSHM_CROSS_FUN
  void reserve(size_t new_size) {
    if (size_) {
      HSHM_THROW_STD_ERROR("Blobs are not meant to be resized after creation");
    } else {
      data_ = CHI_CLIENT->AllocateBuffer(HSHM_MCTX, new_size);
    }
    max_size_ = new_size;
  }

  /** Get the data */
  HSHM_INLINE_CROSS_FUN
  char *data() { return data_.ptr_; }

  /** Get the SHM pointer */
  HSHM_INLINE_CROSS_FUN
  hipc::Pointer &shm() { return data_.shm_; }

  /** Get the SHM pointer */
  HSHM_INLINE_CROSS_FUN
  const hipc::Pointer &shm() const { return data_.shm_; }

  /** Get the size */
  HSHM_INLINE_CROSS_FUN
  size_t size() const { return size_; }

  /** Get the data */
  HSHM_INLINE_CROSS_FUN
  char *data() const { return data_.ptr_; }

  /** Equality */
  HSHM_INLINE_CROSS_FUN
  bool operator==(const Blob &other) const {
    if (size_ != other.size_) {
      return false;
    }
    return memcmp(data_.ptr_, other.data_.ptr_, size_) == 0;
  }

  /** Inequality */
  HSHM_INLINE_CROSS_FUN
  bool operator!=(const Blob &other) const { return !(*this == other); }
};

/** Supported data placement policies */
enum class PlacementPolicy {
  kRandom,         /**< Random blob placement */
  kRoundRobin,     /**< Round-Robin (around devices) blob placement */
  kMinimizeIoTime, /**< LP-based blob placement, minimize I/O time */
  kNone,           /**< No Dpe for cases we want it disabled */
};

/** A class to convert placement policy enum value to string */
class PlacementPolicyConv {
public:
  /** A function to return string representation of \a policy */
  static std::string to_str(PlacementPolicy policy) {
    switch (policy) {
    case PlacementPolicy::kRandom: {
      return "PlacementPolicy::kRandom";
    }
    case PlacementPolicy::kRoundRobin: {
      return "PlacementPolicy::kRoundRobin";
    }
    case PlacementPolicy::kMinimizeIoTime: {
      return "PlacementPolicy::kMinimizeIoTime";
    }
    case PlacementPolicy::kNone: {
      return "PlacementPolicy::kNone";
    }
    }
    return "PlacementPolicy::Invalid";
  }

  /** return enum value of \a policy  */
  static PlacementPolicy to_enum(const std::string &policy) {
    if (policy.find("Random") != std::string::npos) {
      return PlacementPolicy::kRandom;
    } else if (policy.find("RoundRobin") != std::string::npos) {
      return PlacementPolicy::kRoundRobin;
    } else if (policy.find("MinimizeIoTime") != std::string::npos) {
      return PlacementPolicy::kMinimizeIoTime;
    } else if (policy.find("None") != std::string::npos) {
      return PlacementPolicy::kNone;
    }
    return PlacementPolicy::kNone;
  }
};

/** Hermes API call context */
struct Context {
  /** Memory context */
  hipc::MemContext mctx_;

  /** Data placement engine */
  PlacementPolicy dpe_;

  /** The blob's score */
  float blob_score_;

  /** Flags */
  bitfield32_t flags_;

  /** Custom bucket parameters */
  chi::string bkt_params_;

  /** The node id the blob will be accessed from */
  u32 node_id_;

  HSHM_CROSS_FUN
  Context()
      : mctx_(HSHM_MCTX), dpe_(PlacementPolicy::kNone), blob_score_(1),
        node_id_(0) {}
};

/**
 * Represents the fraction of a blob to place
 * on a particular target during data placement
 * */
struct SubPlacement {
  size_t size_;  /**< Size (bytes) */
  TargetId tid_; /**< Index in the target vector */

  SubPlacement() = default;

  explicit SubPlacement(size_t size, const TargetId &tid)
      : size_(size), tid_(tid) {}
};

/**
 * The organization of a particular blob in the storage
 * hierarchy during data placement.
 */
struct PlacementSchema {
  std::vector<SubPlacement> plcmnts_;

  void AddSubPlacement(size_t size, const TargetId &tid) {
    plcmnts_.emplace_back(size, tid);
  }

  void Clear() { plcmnts_.clear(); }
};

/** The types of topologies */
enum class TopologyType {
  Local,
  Neighborhood,
  Global,

  kCount
};

/** The flushing mode */
enum class FlushingMode { kSync, kAsync };

/** Convert flushing modes to strings */
class FlushingModeConv {
public:
  static FlushingMode GetEnum(const std::string &str) {
    if (str == "kSync") {
      return FlushingMode::kSync;
    }
    if (str == "kAsync") {
      return FlushingMode::kAsync;
    }
    return FlushingMode::kAsync;
  }
};

/** A class with static constants */
#define CONST_T static inline const
class Constant {
public:
  /** Hermes server environment variable */
  CONST_T char *kHermesServerConf = "HERMES_CONF";

  /** Hermes client environment variable */
  CONST_T char *kHermesClientConf = "HERMES_CLIENT_CONF";

  /** Hermes adapter mode environment variable */
  CONST_T char *kHermesAdapterMode = "HERMES_ADAPTER_MODE";

  /** Filesystem page size environment variable */
  CONST_T char *kHermesPageSize = "HERMES_PAGE_SIZE";

  /** Stop daemon environment variable */
  CONST_T char *kHermesStopDaemon = "HERMES_STOP_DAEMON";

  /** Maximum path length environment variable */
  CONST_T size_t kMaxPathLength = 4096;
};

/** Represents an allocated fraction of a target */
struct BufferInfo : public chi::Block {
  TargetId tid_; /**< The destination target */

  /** Serialization */
  template <typename Ar> void serialize(Ar &ar) { ar(tid_, off_, size_); }

  /** Default constructor */
  BufferInfo() = default;

  /** Primary constructor */
  BufferInfo(TargetId tid, const chi::Block &block)
      : tid_(tid), chi::Block(block) {}
};

/** Data structure used to store Blob information */
struct BlobInfo {
  TagId tag_id_;                    /**< Tag the blob is on */
  BlobId blob_id_;                  /**< Unique ID of the blob */
  chi::string name_;                /**< Name of the blob (without tag_id) */
  std::vector<BufferInfo> buffers_; /**< Set of buffers */
  std::vector<TagId> tags_;         /**< Set of tags */
  size_t blob_size_;                /**< The overall size of the blob */
  size_t max_blob_size_; /**< The amount of space current buffers support */
  float score_;          /**< The priority of this blob */
  float user_score_;     /**< The user-defined priority of this blob */
  hipc::atomic<u32> access_freq_; /**< Number of times blob accessed in epoch */
  hshm::Timepoint last_access_;   /**< Last time blob accessed */
  hipc::atomic<hshm::big_uint>
      mod_count_; /**< The number of times blob modified */
  hipc::atomic<hshm::big_uint>
      last_flush_;     /**< The last mod that was flushed */
  bitfield32_t flags_; /**< Flags */
#ifdef CHIMAERA_RUNTIME
  chi::CoRwLock lock_; /**< Lock */
#endif

  /** Serialization */
  template <typename Ar> void serialize(Ar &ar) {
    ar(tag_id_, blob_id_, name_, buffers_, tags_, blob_size_, max_blob_size_,
       score_, access_freq_, mod_count_, last_flush_);
  }

  /** Default constructor */
  BlobInfo() = default;

  /** Copy constructor */
  BlobInfo(const BlobInfo &other) {
    tag_id_ = other.tag_id_;
    blob_id_ = other.blob_id_;
    name_ = other.name_;
    buffers_ = other.buffers_;
    tags_ = other.tags_;
    blob_size_ = other.blob_size_;
    max_blob_size_ = other.max_blob_size_;
    score_ = other.score_;
    user_score_ = other.user_score_;
    access_freq_ = other.access_freq_.load();
    last_access_ = other.last_access_;
    mod_count_ = other.mod_count_.load();
    last_flush_ = other.last_flush_.load();
  }

  /** Update modify stats */
  void UpdateWriteStats() {
    mod_count_.fetch_add(1);
    UpdateReadStats();
  }

  /** Update read stats */
  void UpdateReadStats() {
    last_access_.Now();
    access_freq_.fetch_add(1);
  }

  /** Get the globally unique blob name */
  static const chi::string GetBlobNameWithBucket(const TagId &tag_id,
                                                 const chi::string &blob_name) {
    chi::string new_name(sizeof(TagId) + blob_name.size());
    hipc::LocalSerialize srl(new_name);
    srl << tag_id;
    srl << blob_name;
    return new_name;
  }

  /** Return the unique blob name for blob_id_map */
  chi::string GetBlobNameWithBucket() {
    return GetBlobNameWithBucket(tag_id_, name_);
  }

  /** Get std::string of name */
  std::string GetName() { return name_.str(); }
};

/** Data structure used to store Bucket information */
struct TagInfo {
  TagId tag_id_;
  chi::string name_;
  std::list<BlobId> blobs_;
  std::list<Task *> traits_;
  size_t internal_size_;
  size_t page_size_;
  bitfield32_t flags_;
  bool owner_;
  // chi::CoRwLock lock_;

  /** Serialization */
  template <typename Ar> void serialize(Ar &ar) {
    ar(tag_id_, name_, internal_size_, page_size_, owner_, flags_);
  }

  /** Get std::string of name */
  std::vector<char> GetName() {
    std::vector<char> data(name_.size());
    memcpy(data.data(), name_.data(), name_.size());
    return data;
  }
};

/** The mode used to update size */
class UpdateSizeMode {
public:
  TASK_METHOD_T kAdd = 0;
  TASK_METHOD_T kCap = 1;
};

/** The types of I/O that can be performed (for IoCall RPC) */
enum class IoType { kRead, kWrite, kNone };

/** Indicates a PUT or GET for a particular blob */
struct IoStat {
  IoType type_;
  BlobId blob_id_;
  TagId tag_id_;
  size_t blob_size_;
  int rank_;
  hshm::qtok_id id_;

  /** Default constructor */
  IoStat() = default;

  /** Emplace constructor */
  IoStat(IoType type, const BlobId &blob_id, const TagId &tag_id,
         size_t blob_size, int rank)
      : type_(type), blob_id_(blob_id), tag_id_(tag_id), blob_size_(blob_size),
        rank_(rank) {}

  /** Copy constructor */
  IoStat(const IoStat &other) { Copy(other); }

  /** Copy assignment */
  IoStat &operator=(const IoStat &other) {
    if (this != &other) {
      Copy(other);
    }
    return *this;
  }

  /** Move constructor */
  IoStat(IoStat &&other) { Copy(other); }

  /** Move assignment */
  IoStat &operator=(IoStat &&other) {
    if (this != &other) {
      Copy(other);
    }
    return *this;
  }

  /** Generic copy / move */
  HSHM_INLINE void Copy(const IoStat &other) {
    type_ = other.type_;
    blob_id_ = other.blob_id_;
    tag_id_ = other.tag_id_;
    blob_size_ = other.blob_size_;
    rank_ = other.rank_;
    id_ = other.id_;
  }

  /** Serialize  */
  template <typename Ar> void serialize(Ar &ar) {
    ar(type_, blob_id_, tag_id_, blob_size_, rank_, id_);
  }
};

/** Used for debugging concurrency issues with locks */
enum LockOwners {
  kNone = 0,
  kMDM_Create = 1,
  kMDM_Update = 2,
  kMDM_Find = 3,
  kMDM_Find2 = 4,
  kMDM_Delete = 5,
  kFS_GetBaseAdapterMode = 6,
  kFS_GetAdapterMode = 7,
  kFS_GetAdapterPageSize = 8,
  kBORG_LocalEnqueueFlushes = 9,
  kBORG_LocalProcessFlushes = 10,
  kMDM_LocalGetBucketSize = 12,
  kMDM_LocalSetBucketSize = 13,
  kMDM_LocalLockBucket = 14,
  kMDM_LocalUnlockBucket = 15,
  kMDM_LocalClearBucket = 16,
  kMDM_LocalTagBlob = 17,
  kMDM_LocalBlobHasTag = 18,
  kMDM_LocalTryCreateBlob = 19,
  kMDM_LocalPutBlobMetadata = 20,
  kMDM_LocalGetBlobId = 21,
  kMDM_LocalGetBlobName = 22,
  kMDM_LocalGetBlobScore = 24,
  kMDM_LocalLockBlob = 26,
  kMDM_LocalUnlockBlob = 27,
  kMDM_LocalGetBlobBuffers = 28,
  kMDM_LocalRenameBlob = 29,
  kMDM_LocalDestroyBlob = 30,
  kMDM_LocalClear = 31,
  kMDM_LocalGetOrCreateTag = 32,
  kMDM_LocalGetTagId = 33,
  kMDM_LocalGetTagName = 34,
  kMDM_LocalRenameTag = 35,
  kMDM_LocalDestroyTag = 36,
  kMDM_LocalTagAddBlob = 37,
  kMDM_LocalTagRemoveBlob = 38,
  kMDM_LocalGroupByTag = 39,
  kMDM_LocalTagAddTrait = 40,
  kMDM_LocalTagGetTraits = 41,
  kMDM_LocalRegisterTrait = 42,
  kMDM_LocalGetTraitId = 43,
  kMDM_LocalGetTraitParams = 44,
  kMDM_GlobalGetTrait = 45,
  kMDM_AddIoStat = 46,
  kMDM_ClearIoStats = 47
};

} // namespace hermes

#endif // HRUN_TASKS_HERMES_INCLUDE_HERMES_HERMES_TYPES_H_
