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

#include <string>

#include "bdev/bdev_client.h"
#include "chimaera/api/chimaera_runtime.h"
#include "chimaera/chimaera_types.h"
#include "chimaera/monitor/monitor.h"
#include "chimaera/work_orchestrator/work_orchestrator.h"
#include "chimaera_admin/chimaera_admin_client.h"
#include "hermes/data_stager/stager_factory.h"
#include "hermes/dpe/dpe_factory.h"
#include "hermes/hermes.h"
#include "hermes_core/hermes_core_client.h"

/** NOTE(llogan): std::hash function for string. This is because NVCC is bugged
 */
namespace std {
template <> struct hash<chi::string> {
  size_t operator()(const chi::string &text) const { return text.Hash(); }
};
} // namespace std

namespace hermes {

#define HERMES_LANES 32

struct FlushInfo {
  BlobInfo *blob_info_;
  FullPtr<StageOutTask> stage_task_;
  size_t mod_count_;
};

/** Type name simplification for the various map types */
typedef std::unordered_map<chi::string, TagId> TAG_ID_MAP_T;
typedef std::unordered_map<TagId, TagInfo> TAG_MAP_T;
typedef std::unordered_map<chi::string, BlobId> BLOB_ID_MAP_T;
typedef std::unordered_map<BlobId, BlobInfo> BLOB_MAP_T;
typedef hipc::circular_mpsc_queue<IoStat> IO_PATTERN_LOG_T;
typedef std::unordered_map<TagId, std::shared_ptr<AbstractStager>> STAGER_MAP_T;

struct HermesLane {
  TAG_ID_MAP_T tag_id_map_;
  TAG_MAP_T tag_map_;
  BLOB_ID_MAP_T blob_id_map_;
  BLOB_MAP_T blob_map_;
  STAGER_MAP_T stager_map_;
  chi::CoMutex stager_map_lock_;
  chi::CoRwLock tag_map_lock_;
  chi::CoRwLock blob_map_lock_;
};

class Server : public Module {
public:
  CLS_CONST LaneGroupId kDefaultGroup = 0;
  Client client_;
  std::vector<HermesLane> tls_;
  std::atomic<u64> id_alloc_;
  std::vector<chi::bdev::Client> tgt_pools_;
  std::vector<TargetInfo> targets_;
  std::unordered_map<TargetId, TargetInfo *> target_map_;
  chi::RollingAverage monitor_[Method::kCount];
  IO_PATTERN_LOG_T io_pattern_;
  TargetInfo *fallback_target_;

private:
  /** Get the globally unique blob name */
  const chi::string GetBlobNameWithBucket(const TagId &tag_id,
                                          const chi::string &blob_name) {
    return BlobInfo::GetBlobNameWithBucket(tag_id, blob_name);
  }

public:
  Server() = default;

  CHI_BEGIN(Create)
  /** Create target pools */
  void CreateTargetPools() {
    for (DeviceInfo &dev : HERMES_SERVER_CONF.devices_) {
      dev.mount_point_ =
          hshm::Formatter::format("{}/{}", dev.mount_dir_, dev.dev_name_);
      HILOG(kInfo, "Creating target: {}", dev.dev_name_);
      chi::bdev::Client target;
      target.Create(
          HSHM_MCTX,
          DomainQuery::GetDirectHash(chi::SubDomainId::kGlobalContainers, 0),
          DomainQuery::GetGlobalBcast(),
          hshm::Formatter::format("hermes_{}", dev.dev_name_), dev.mount_point_,
          dev.capacity_);
      tgt_pools_.emplace_back(target);
    }
  }
  /** Create target neighborhood */
  void CreateTargetNeighborhood() {
    // NOTE(llogan): The vector here is a bit dangerous because of the
    // map assignment. We need to reserve here, or else the vector
    // will resize and cause the map to be erronous.
    targets_.reserve(128);
    size_t dev_idx = 0;
    for (chi::bdev::Client &tgt_pool : tgt_pools_) {
      targets_.emplace_back();
      TargetInfo &target = targets_.back();
      target.client_ = tgt_pool;
      target.dom_query_ = DomainQuery::GetLocalHash(0);
      target.id_ = target.client_.pool_id_;
      target.id_.node_id_ = CHI_CLIENT->node_id_;
      if (target_map_.find(target.id_) != target_map_.end()) {
        targets_.pop_back();
        continue;
      }
      HILOG(kInfo, "(node {}) Created target: {}", CHI_CLIENT->node_id_,
            target.id_);
      // Poll stats periodically
      target.poll_stats_ =
          target.client_.AsyncPollStats(HSHM_MCTX, target.dom_query_, 25);
      HILOG(kInfo, "Polling stats periodically {}",
            target.poll_stats_->task_node_);
      // Get current stats for bdevs
      target.poll_stats_->stats_ =
          target.client_.PollStats(HSHM_MCTX, target.dom_query_);
      target.stats_ = &target.poll_stats_->stats_;
      target.score_ = HERMES_SERVER_CONF.devices_[dev_idx].score_;
      target_map_[target.id_] = &target;
      HILOG(kInfo, "Got stats for target: {}", target.id_);
      dev_idx++;
    }
    // TODO(llogan): We should sort targets first
    fallback_target_ = &targets_.back();
  }
  /** Construct hermes_core */
  void Create(CreateTask *task, RunContext &rctx) {
    // Create a set of lanes for holding tasks
    HERMES_CONF->ServerInit();
    client_.Init(pool_id_);
    CreateLaneGroup(kDefaultGroup, HERMES_LANES, QUEUE_LOW_LATENCY);
    tls_.resize(HERMES_LANES);
    io_pattern_.resize(8192);
    CreateTargetPools();
    CreateTargetNeighborhood();
    client_.AsyncFlushData(
        HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
        HERMES_CONF->server_config_.borg_.flush_period_); // OK
  }
  void MonitorCreate(MonitorModeId mode, CreateTask *task, RunContext &rctx) {}
  CHI_END(Create)

  /** Route a task to a lane */
  Lane *MapTaskToLane(const Task *task) override {
    // Route tasks to lanes based on their properties
    // E.g., a strongly consistent filesystem could map tasks to a lane
    // by the hash of an absolute filename path.

    // Can I route put / get tasks to nodes here? I feel like yes.

    return GetLaneByHash(kDefaultGroup, task->prio_, 0);
  }

  CHI_BEGIN(Destroy)
  /** Destroy hermes_core */
  void Destroy(DestroyTask *task, RunContext &rctx) {}
  void MonitorDestroy(MonitorModeId mode, DestroyTask *task, RunContext &rctx) {
  }
  CHI_END(Create)

  /**
   * ========================================
   * CACHING Methods
   * ========================================
   * */

  /** Get blob info struct */
  BlobInfo *GetBlobInfo(const std::string &blob_name, BlobId blob_id) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    BLOB_MAP_T &blob_map = tls.blob_map_;
    // Check if blob name is cached on this node
    if (!blob_name.empty()) {
      auto it = blob_id_map.find(blob_name);
      if (it == blob_id_map.end()) {
        return nullptr;
      }
      blob_id = it->second;
    }
    // Check if blob ID is cached on this node
    if (!blob_id.IsNull()) {
      auto it = blob_map.find(blob_id);
      if (it != blob_map.end()) {
        return &it->second;
      }
    }
    return nullptr;
  }

  /** Get tag info struct */
  TagInfo *GetTagInfo(const std::string &tag_name, TagId tag_id) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    TAG_ID_MAP_T &tag_id_map = tls.tag_id_map_;
    TAG_MAP_T &tag_map = tls.tag_map_;
    // Check if tag name is cached on this node
    if (!tag_name.empty()) {
      auto it = tag_id_map.find(tag_name);
      if (it == tag_id_map.end()) {
        return nullptr;
      }
      tag_id = it->second;
    }
    // Check if tag ID is cached on this node
    if (!tag_id.IsNull()) {
      auto it = tag_map.find(tag_id);
      if (it != tag_map.end()) {
        return &it->second;
      }
    }
    return nullptr;
  }

  template <typename TaskT> void BlobCacheWriteRoute(TaskT *task) {
    std::string blob_name;
    BlobId blob_id(BlobId::GetNull());
    TagId tag_id(task->tag_id_);
    if constexpr (std::is_base_of_v<BlobWithId, TaskT>) {
      blob_id = task->blob_id_;
    }
    if constexpr (std::is_base_of_v<BlobWithName, TaskT>) {
      blob_name = task->blob_name_.str();
    }
    BlobInfo *blob_info = GetBlobInfo(blob_name, blob_id);
    if (blob_info || task->IsDirect()) {
      return;
    }
    u32 name_hash = HashBlobName(tag_id, blob_name);
    u32 id_hash = hshm::hash<BlobId>{}(blob_id);
    u32 hash = HashBlobNameOrId(tag_id, blob_name, blob_id);
    task->dom_query_ = chi::DomainQuery::GetDirectHash(
        chi::SubDomainId::kGlobalContainers, hash);
    // HILOG(kInfo,
    //       "(node {}) Routing to: hash={} (mod8={} blob_id={} name={} len={},
    //       " "name_hash={} id_hash={})", CHI_CLIENT->node_id_, hash,
    //       task->dom_query_.sel_.hash_ % 8, blob_id, blob_name,
    //       blob_name.size(), name_hash, id_hash);
    task->SetDirect();
    task->UnsetRouted();
    // HILOG(kInfo, "Routing to: {}", task->dom_query_);
  }

  template <typename TaskT> void BlobCacheReadRoute(TaskT *task) {
    std::string blob_name;
    BlobId blob_id(BlobId::GetNull());
    TagId tag_id(task->tag_id_);
    if constexpr (std::is_base_of_v<BlobWithId, TaskT>) {
      blob_id = task->blob_id_;
    }
    if constexpr (std::is_base_of_v<BlobWithName, TaskT>) {
      blob_name = task->blob_name_.str();
    }
    BlobInfo *blob_info = GetBlobInfo(blob_name, blob_id);
    if (blob_info || task->IsDirect()) {
      return;
    }
    u32 name_hash = HashBlobName(tag_id, blob_name);
    u32 id_hash = hshm::hash<BlobId>{}(blob_id);
    u32 hash = HashBlobNameOrId(tag_id, blob_name, blob_id);
    task->dom_query_ = chi::DomainQuery::GetDirectHash(
        chi::SubDomainId::kGlobalContainers, hash);
    // HILOG(kInfo,
    //       "(node {}) Routing to: hash={} (mod8={} blob_id={} name={} len={},
    //       " "name_hash={} id_hash={})", CHI_CLIENT->node_id_, hash,
    //       task->dom_query_.sel_.hash_ % 8, blob_id, blob_name,
    //       blob_name.size(), name_hash, id_hash);
    task->SetDirect();
    task->UnsetRouted();
  }

  template <typename TaskT> void TagCacheWriteRoute(TaskT *task) {
    std::string tag_name;
    TagId tag_id(TagId::GetNull());
    if constexpr (std::is_base_of_v<TagWithId, TaskT>) {
      tag_id = task->tag_id_;
    }
    if constexpr (std::is_base_of_v<TagWithName, TaskT>) {
      tag_name = task->tag_name_.str();
    }
    // HILOG(kInfo, "TAG NAME: {}", tag_name);
    // HILOG(kInfo, "TAG ID: {}", tag_id);
    TagInfo *tag_info = GetTagInfo(tag_name, tag_id);
    if (tag_info || task->IsDirect()) {
      // HILOG(kInfo, "Tag existed");
      return;
    }
    // HILOG(kInfo, "Tag did not exist");
    task->SetDirect();
    task->UnsetRouted();
    u32 name_hash = HashTagName(tag_name);
    u32 id_hash = hshm::hash<TagId>{}(tag_id);
    u32 hash = HashTagNameOrId(tag_id, tag_name);
    task->dom_query_ = chi::DomainQuery::GetDirectHash(
        chi::SubDomainId::kGlobalContainers, hash);
    // HILOG(kInfo,
    //       "(node {}) Routing to: hash={} (mod8={} tag_id={} name={} len={}, "
    //       "name_hash={} id_hash={})",
    //       CHI_CLIENT->node_id_, hash, task->dom_query_.sel_.hash_ % 8,
    //       tag_id, tag_name, tag_name.size(), name_hash, id_hash);
    task->SetDirect();
    task->UnsetRouted();
  }

  template <typename TaskT> void TagCacheReadRoute(TaskT *task) {
    std::string tag_name;
    TagId tag_id(TagId::GetNull());
    if constexpr (std::is_base_of_v<TagWithId, TaskT>) {
      tag_id = task->tag_id_;
    }
    if constexpr (std::is_base_of_v<TagWithName, TaskT>) {
      tag_name = task->tag_name_.str();
    }
    TagInfo *tag_info = GetTagInfo(tag_name, tag_id);
    if (tag_info || task->IsDirect()) {
      return;
    }
    task->dom_query_ = chi::DomainQuery::GetDirectHash(
        chi::SubDomainId::kGlobalContainers, HashTagNameOrId(tag_id, tag_name));
    task->SetDirect();
    task->UnsetRouted();
    u32 name_hash = HashTagName(tag_name);
    u32 id_hash = hshm::hash<TagId>{}(tag_id);
    u32 hash = HashTagNameOrId(tag_id, tag_name);
    task->dom_query_ = chi::DomainQuery::GetDirectHash(
        chi::SubDomainId::kGlobalContainers, hash);
    // HILOG(kInfo,
    //       "(node {}) Routing to: hash={} (mod8={} tag_id={} name={} len={}, "
    //       "name_hash={} id_hash={})",
    //       CHI_CLIENT->node_id_, hash, task->dom_query_.sel_.hash_ % 8,
    //       tag_id, tag_name, tag_name.size(), name_hash, id_hash);
    task->SetDirect();
    task->UnsetRouted();
  }

  void PutBlobBegin(PutBlobTask *task, char *data, size_t data_size,
                    RunContext &rctx) {}

  void PutBlobEnd(PutBlobTask *task, RunContext &rctx) {}

  void GetBlobBegin(GetBlobTask *task, RunContext &rctx) {}

  void GetBlobEnd(GetBlobTask *task, RunContext &rctx) {}

  /**
   * ========================================
   * TAG Methods
   * ========================================
   * */

  CHI_BEGIN(GetOrCreateTag)
  /** Get or create a tag */
  void GetOrCreateTag(GetOrCreateTagTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_ID_MAP_T &tag_id_map = tls.tag_id_map_;
    TAG_MAP_T &tag_map = tls.tag_map_;
    chi::string tag_name(task->tag_name_);
    bool did_create = false;
    if (tag_name.size() > 0) {
      did_create = tag_id_map.find(tag_name) == tag_id_map.end();
    }

    // Emplace bucket if it does not already exist
    TagId tag_id;
    if (did_create) {
      TAG_MAP_T &tag_map = tls.tag_map_;
      tag_id.unique_ = id_alloc_.fetch_add(1);
      tag_id.hash_ = HashTagName(tag_name);
      tag_id.node_id_ = CHI_CLIENT->node_id_;
      HILOG(kInfo, "Creating tag for the first time: {} {}", tag_name.str(),
            tag_id);
      tag_id_map.emplace(tag_name, tag_id);
      tag_map.emplace(tag_id, TagInfo());
      TagInfo &tag = tag_map[tag_id];
      tag.name_ = tag_name;
      tag.tag_id_ = tag_id;
      tag.owner_ = task->blob_owner_;
      tag.internal_size_ = task->backend_size_;
      if (task->flags_.Any(HERMES_SHOULD_STAGE)) {
        client_.RegisterStager(HSHM_MCTX, chi::DomainQuery::GetGlobalBcast(),
                               tag_id, chi::string(task->tag_name_.str()),
                               chi::string(task->params_.str()));
        tag.flags_.SetBits(HERMES_SHOULD_STAGE);
      }
    } else {
      if (tag_name.size()) {
        HILOG(kInfo, "Found existing tag: {}", tag_name.str());
        tag_id = tag_id_map[tag_name];
      } else {
        HILOG(kInfo, "Found existing tag: {}", task->tag_id_);
        tag_id = task->tag_id_;
      }
    }

    task->tag_id_ = tag_id;
    // task->did_create_ = did_create;
  }
  void MonitorGetOrCreateTag(MonitorModeId mode, GetOrCreateTagTask *task,
                             RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheWriteRoute<GetOrCreateTagTask>(task);
      return;
    }
    }
  }
  CHI_END(GetOrCreateTag)

  CHI_BEGIN(GetTagId)
  /** Get an existing tag ID */
  void GetTagId(GetTagIdTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_ID_MAP_T &tag_id_map = tls.tag_id_map_;
    chi::string tag_name(task->tag_name_);
    auto it = tag_id_map.find(tag_name);
    if (it == tag_id_map.end()) {
      task->tag_id_ = TagId::GetNull();
      return;
    }
    task->tag_id_ = it->second;
  }
  void MonitorGetTagId(MonitorModeId mode, GetTagIdTask *task,
                       RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheReadRoute<GetTagIdTask>(task);
      return;
    }
    }
  }
  CHI_END(GetTagId)

  CHI_BEGIN(GetTagName)
  /** Get the name of a tag */
  void GetTagName(GetTagNameTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    task->tag_name_ = it->second.name_;
  }
  void MonitorGetTagName(MonitorModeId mode, GetTagNameTask *task,
                         RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheReadRoute<GetTagNameTask>(task);
      return;
    }
    }
  }
  CHI_END(GetTagName)

  CHI_BEGIN(DestroyTag)
  /** Destroy a tag */
  void DestroyTag(DestroyTagTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwWriteLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    TagInfo &tag = it->second;
    if (tag.owner_) {
      for (BlobId &blob_id : tag.blobs_) {
        client_.AsyncDestroyBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                                 task->tag_id_, blob_id,
                                 DestroyBlobTask::kKeepInTag,
                                 TASK_FIRE_AND_FORGET); // TODO(llogan): route
      }
    }
    if (tag.flags_.Any(HERMES_SHOULD_STAGE)) {
      client_.UnregisterStager(HSHM_MCTX, chi::DomainQuery::GetGlobalBcast(),
                               task->tag_id_); // OK
    }
    // Remove tag from maps
    TAG_ID_MAP_T &tag_id_map = tls.tag_id_map_;
    tag_id_map.erase(tag.name_);
    tag_map.erase(it);
  }
  void MonitorDestroyTag(MonitorModeId mode, DestroyTagTask *task,
                         RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheWriteRoute<DestroyTagTask>(task);
      return;
    }
    }
  }
  CHI_END(DestroyTag)

  CHI_BEGIN(TagAddBlob)
  /** Add a blob to the tag */
  void TagAddBlob(TagAddBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    TagInfo &tag = it->second;
    tag.blobs_.emplace_back(task->blob_id_);
  }
  void MonitorTagAddBlob(MonitorModeId mode, TagAddBlobTask *task,
                         RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheWriteRoute<TagAddBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(TagAddBlob)

  CHI_BEGIN(TagRemoveBlob)
  /** Remove a blob from the tag */
  void TagRemoveBlob(TagRemoveBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    TagInfo &tag = it->second;
    auto blob_it =
        std::find(tag.blobs_.begin(), tag.blobs_.end(), task->blob_id_);
    tag.blobs_.erase(blob_it);
  }
  void MonitorTagRemoveBlob(MonitorModeId mode, TagRemoveBlobTask *task,
                            RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheWriteRoute<TagRemoveBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(TagRemoveBlob)

  CHI_BEGIN(TagClearBlobs)
  /** Clear blobs from the tag */
  void TagClearBlobs(TagClearBlobsTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    TagInfo &tag = it->second;
    if (tag.owner_) {
      for (BlobId &blob_id : tag.blobs_) {
        client_.AsyncDestroyBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                                 task->tag_id_, blob_id,
                                 DestroyBlobTask::kKeepInTag,
                                 TASK_FIRE_AND_FORGET); // TODO(llogan): route
      }
    }
    tag.blobs_.clear();
    tag.internal_size_ = 0;
  }
  void MonitorTagClearBlobs(MonitorModeId mode, TagClearBlobsTask *task,
                            RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheWriteRoute<TagClearBlobsTask>(task);
      return;
    }
    }
  }
  CHI_END(TagClearBlobs)

  CHI_BEGIN(TagGetSize)
  /** Get the size of a tag */
  void TagGetSize(TagGetSizeTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      task->size_ = 0;
      return;
    }
    TagInfo &tag = it->second;
    task->size_ = tag.internal_size_;
  }
  void MonitorTagGetSize(MonitorModeId mode, TagGetSizeTask *task,
                         RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheReadRoute<TagGetSizeTask>(task);
      return;
    }
    }
  }
  CHI_END(TagGetSize)

  CHI_BEGIN(TagUpdateSize)
  /** Update the size of a tag */
  void TagUpdateSize(TagUpdateSizeTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    TagInfo &tag = tag_map[task->tag_id_];
    ssize_t internal_size = (ssize_t)tag.internal_size_;
    if (task->mode_ == UpdateSizeMode::kAdd) {
      internal_size += task->update_;
    } else {
      internal_size = std::max(task->update_, internal_size);
    }
    HILOG(kDebug,
          "Updating size of tag {} from {} to {} with update {} (mode={})",
          task->tag_id_, tag.internal_size_, internal_size, task->update_,
          task->mode_);
    tag.internal_size_ = (size_t)internal_size;
  }
  void MonitorTagUpdateSize(MonitorModeId mode, TagUpdateSizeTask *task,
                            RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheWriteRoute<TagUpdateSizeTask>(task);
      return;
    }
    }
  }
  CHI_END(TagUpdateSize)

  CHI_BEGIN(TagUpdateSize)
  /** Get the set of blobs in the tag */
  void TagGetContainedBlobIds(TagGetContainedBlobIdsTask *task,
                              RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    TagInfo &tag = it->second;
    chi::ipc::vector<BlobId> &blobs = task->blob_ids_;
    blobs.reserve(tag.blobs_.size());
    for (BlobId &blob_id : tag.blobs_) {
      blobs.emplace_back(blob_id);
    }
  }
  void MonitorTagGetContainedBlobIds(MonitorModeId mode,
                                     TagGetContainedBlobIdsTask *task,
                                     RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      TagCacheReadRoute<TagGetContainedBlobIdsTask>(task);
      return;
    }
    }
  }
  CHI_END(TagGetContainedBlobIds)

  CHI_BEGIN(TagFlush)
  /** Flush tag */
  void TagFlush(TagFlushTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    auto it = tag_map.find(task->tag_id_);
    if (it == tag_map.end()) {
      return;
    }
    TagInfo &tag = it->second;
    for (BlobId &blob_id : tag.blobs_) {
      client_.FlushBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                        blob_id); // TODO(llogan): route
    }
    // Flush blobs
  }
  void MonitorTagFlush(MonitorModeId mode, TagFlushTask *task,
                       RunContext &rctx) {}
  CHI_END(TagUpdateSize)

  /**
   * ========================================
   * BLOB Methods
   * ========================================
   * */

  /** Get or create a blob ID */
  BlobId GetOrCreateBlobId(HermesLane &tls, TagId &tag_id, u32 name_hash,
                           const chi::string &blob_name, bitfield32_t &flags) {
    chi::string blob_name_unique = GetBlobNameWithBucket(tag_id, blob_name);
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    auto it = blob_id_map.find(blob_name_unique);
    if (it == blob_id_map.end()) {
      BlobId blob_id =
          BlobId(CHI_CLIENT->node_id_, name_hash, id_alloc_.fetch_add(1));
      blob_id_map.emplace(blob_name_unique, blob_id);
      flags.SetBits(HERMES_BLOB_DID_CREATE);
      BLOB_MAP_T &blob_map = tls.blob_map_;
      blob_map.emplace(blob_id, BlobInfo());
      BlobInfo &blob_info = blob_map[blob_id];
      blob_info.name_ = blob_name;
      blob_info.blob_id_ = blob_id;
      blob_info.tag_id_ = tag_id;
      blob_info.blob_size_ = 0;
      blob_info.max_blob_size_ = 0;
      blob_info.score_ = 1;
      blob_info.mod_count_ = 0;
      blob_info.access_freq_ = 0;
      blob_info.last_flush_ = 0;
      blob_info.flags_ = flags;
      return blob_id;
    }
    return it->second;
  }

  CHI_BEGIN(GetOrCreateBlobId)
  /** Get or create a blob ID */
  void GetOrCreateBlobId(GetOrCreateBlobIdTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    chi::string blob_name(task->blob_name_);
    bitfield32_t flags;
    task->blob_id_ = GetOrCreateBlobId(tls, task->tag_id_,
                                       HashBlobName(task->tag_id_, blob_name),
                                       blob_name, flags);
  }
  void MonitorGetOrCreateBlobId(MonitorModeId mode, GetOrCreateBlobIdTask *task,
                                RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetOrCreateBlobIdTask>(task);
      return;
    }
    }
  }
  CHI_END(GetOrCreateBlobId)

  CHI_BEGIN(GetBlobId)
  /** Get the blob ID */
  void GetBlobId(GetBlobIdTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    chi::string blob_name(task->blob_name_);
    chi::string blob_name_unique =
        GetBlobNameWithBucket(task->tag_id_, blob_name);
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    auto it = blob_id_map.find(blob_name_unique);
    if (it == blob_id_map.end()) {
      task->blob_id_ = BlobId::GetNull();
      HILOG(kDebug, "Failed to find blob {} in {}", blob_name.str(),
            task->tag_id_);
      return;
    }
    task->blob_id_ = it->second;
  }
  void MonitorGetBlobId(MonitorModeId mode, GetBlobIdTask *task,
                        RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetBlobIdTask>(task);
      return;
    }
    }
  }
  CHI_END(GetBlobId)

  CHI_BEGIN(GetBlobName)
  /** Get blob name */
  void GetBlobName(GetBlobNameTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob = it->second;
    task->blob_name_ = blob.name_;
  }
  void MonitorGetBlobName(MonitorModeId mode, GetBlobNameTask *task,
                          RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetBlobNameTask>(task);
      return;
    }
    }
  }
  CHI_END(GetBlobName)

  CHI_BEGIN(GetBlobSize)
  /** Get the blob size */
  void GetBlobSize(GetBlobSizeTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    if (task->blob_id_.IsNull()) {
      bitfield32_t flags;
      task->blob_id_ = GetOrCreateBlobId(
          tls, task->tag_id_, HashBlobName(task->tag_id_, task->blob_name_),
          chi::string(task->blob_name_), flags);
    }
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      task->size_ = 0;
      return;
    }
    BlobInfo &blob = it->second;
    task->size_ = blob.blob_size_;
  }
  void MonitorGetBlobSize(MonitorModeId mode, GetBlobSizeTask *task,
                          RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetBlobSizeTask>(task);
      return;
    }
    }
  }
  CHI_END(GetBlobSize)

  CHI_BEGIN(GetBlobScore)
  /** Get the score of a blob */
  void GetBlobScore(GetBlobScoreTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob = it->second;
    task->score_ = blob.score_;
  }
  void MonitorGetBlobScore(MonitorModeId mode, GetBlobScoreTask *task,
                           RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetBlobScoreTask>(task);
      return;
    }
    }
  }
  CHI_END(GetBlobScore)

  CHI_BEGIN(GetBlobBuffers)
  /** Get blob buffers */
  void GetBlobBuffers(GetBlobBuffersTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob = it->second;
    task->buffers_ = blob.buffers_;
  }
  void MonitorGetBlobBuffers(MonitorModeId mode, GetBlobBuffersTask *task,
                             RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetBlobBuffersTask>(task);
      return;
    }
    }
  }
  CHI_END(GetBlobBuffers)

  /** A slice of data in a bigger data element */
  struct Slice {
    size_t off_ = 0;
    size_t size_ = 0;
  };

  /** Used by PutBlob and GetBlob to do partial I/O */
  class DataIterator {
  public:
    Slice part_;         // part of blob to find
    Slice rem_;          // remaining part of blob
    Slice intersect_;    // relative part of buffer
    size_t tgt_off_;     // actual part of target
    size_t data_off_;    // actual part of blob
    size_t cur_off_ = 0; // beginning of current buffer
    size_t cutoff_;      // max value of cur_off

  public:
    DataIterator(const Slice &part) : part_(part), rem_(part) {
      cutoff_ = part_.off_ + part_.size_;
    }

    void Intersect(BufferInfo &buf) {
      // Part of the blob is in this buffer
      if (cur_off_ <= rem_.off_ && rem_.off_ < cur_off_ + buf.size_) {
        // Where in the buffer do we write?
        intersect_.off_ = rem_.off_ - cur_off_;
        intersect_.size_ = buf.size_ - intersect_.off_;
        if (intersect_.size_ > rem_.size_) {
          intersect_.size_ = rem_.size_;
        }
        // Real offsets
        tgt_off_ = buf.off_ + intersect_.off_;
        data_off_ = rem_.off_ - part_.off_;
        // How much of the blob is left?
        rem_.off_ += intersect_.size_;
        rem_.size_ -= intersect_.size_;
      }
      cur_off_ += buf.size_;
    }

    bool DidIntersect() { return intersect_.size_ > 0; }

    bool IsDone() { return cur_off_ >= cutoff_ || rem_.size_ == 0; }
  };

  CHI_BEGIN(PutBlob)
  /** Put a blob */
  void PutBlob(PutBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    // Get blob ID
    chi::string blob_name(task->blob_name_);
    if (task->blob_id_.IsNull()) {
      task->blob_id_ = GetOrCreateBlobId(tls, task->tag_id_,
                                         HashBlobName(task->tag_id_, blob_name),
                                         blob_name, task->flags_);
    }

    // Verify data is non-zero
    if (task->data_size_ == 0) {
      return;
    }

    HILOG(kDebug, "(node {}) Put blob with ID {} data_size={} task_node={}",
          CHI_CLIENT->node_id_, task->blob_id_, task->data_size_,
          task->task_node_);

    // Get blob struct
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob_info = it->second;
    chi::ScopedCoRwWriteLock blob_info_lock(blob_info.lock_);

    // Stage Blob
    if (blob_info.flags_.Any(HERMES_SHOULD_STAGE) &&
        !blob_info.flags_.Any(HERMES_DID_STAGE_IN)) {
      blob_info.flags_.SetBits(HERMES_DID_STAGE_IN);
      client_.StageIn(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                      task->tag_id_, blob_info.name_, 1);
    }

    // Determine amount of additional buffering space needed
    ssize_t bkt_size_diff = 0;
    size_t needed_space = task->blob_off_ + task->data_size_;
    size_t size_diff = 0;
    if (needed_space > blob_info.max_blob_size_) {
      size_diff = needed_space - blob_info.max_blob_size_;
    }
    size_t min_blob_size = task->blob_off_ + task->data_size_;
    if (min_blob_size > blob_info.blob_size_) {
      blob_info.blob_size_ = min_blob_size;
    }
    bkt_size_diff += (ssize_t)size_diff;
    HILOG(kDebug, "The size diff is {} bytes (bkt diff {})", size_diff,
          bkt_size_diff);

    // Use DPE
    std::vector<TargetInfo> targets = targets_;
    std::vector<PlacementSchema> schema_vec;
    if (size_diff > 0) {
      Context ctx;
      auto *dpe = DpeFactory::Get(ctx.dpe_);
      ctx.blob_score_ = task->score_;
      dpe->Placement({size_diff}, targets, ctx, schema_vec);
    }

    // Allocate blob buffers
    for (PlacementSchema &schema : schema_vec) {
      schema.plcmnts_.emplace_back(0, fallback_target_->id_);
      for (size_t sub_idx = 0; sub_idx < schema.plcmnts_.size(); ++sub_idx) {
        // Allocate chi::blocks
        SubPlacement &placement = schema.plcmnts_[sub_idx];
        TargetInfo &target = *target_map_[placement.tid_];
        if (placement.size_ == 0) {
          continue;
        }
        std::vector<chi::Block> blocks = target.client_.Allocate(
            HSHM_MCTX, target.dom_query_, placement.size_);
        // Convert to BufferInfo
        size_t t_alloc = 0;
        for (chi::Block &block : blocks) {
          if (block.size_ == 0) {
            continue;
          }
          blob_info.buffers_.emplace_back(placement.tid_, block);
          t_alloc += block.size_;
        }
        // HILOG(kInfo, "(node {}) Placing {}/{} bytes in target {} of bw {}",
        //       CHI_CLIENT->node_id_, t_alloc, placement.size_, placement.tid_,
        //       target.stats_->write_bw_);
        // Spill to next tier
        size_t next_tier = sub_idx + 1;
        if (t_alloc < placement.size_ && next_tier < schema.plcmnts_.size()) {
          SubPlacement &next_placement = schema.plcmnts_[next_tier];
          size_t diff = placement.size_ - t_alloc;
          next_placement.size_ += diff;
        }
        target.stats_->free_ -= t_alloc;
      }
    }

    // Place blob in buffers
    std::vector<FullPtr<chi::bdev::WriteTask>> write_tasks;
    write_tasks.reserve(blob_info.buffers_.size());
    DataIterator diter(Slice{task->blob_off_, task->data_size_});
    size_t buf_sum = 0;
    for (BufferInfo &buf : blob_info.buffers_) {
      diter.Intersect(buf);
      if (diter.DidIntersect()) {
        // HILOG(kInfo, "Writing {} bytes at off {} from target {}", buf_size,
        //       tgt_off, buf.tid_);
        TargetInfo &target = *target_map_[buf.tid_];
        FullPtr<chi::bdev::WriteTask> write_task = target.client_.AsyncWrite(
            HSHM_MCTX, target.dom_query_, task->data_ + diter.data_off_,
            diter.tgt_off_, diter.intersect_.size_);
        write_tasks.emplace_back(write_task);
      }
      buf_sum += buf.size_;
    }
    blob_info.max_blob_size_ = buf_sum;

    // Wait for the placements to complete
    task->Wait(write_tasks);
    for (FullPtr<chi::bdev::WriteTask> &write_task : write_tasks) {
      CHI_CLIENT->DelTask(HSHM_MCTX, write_task);
    }

    // Update information
    if (task->flags_.Any(HERMES_SHOULD_STAGE)) {
      STAGER_MAP_T &stager_map = tls.stager_map_;
      chi::ScopedCoMutex stager_map_lock(tls.stager_map_lock_);
      auto it = stager_map.find(task->tag_id_);
      if (it == stager_map.end()) {
        HELOG(kWarning, "Could not find stager for tag {}. Not updating size",
              task->tag_id_);
      } else {
        std::shared_ptr<AbstractStager> &stager = it->second;
        stager->UpdateSize(HSHM_MCTX, client_, task->tag_id_,
                           blob_info.name_.str(), task->blob_off_,
                           task->data_size_);
      }
    } else {
      client_.AsyncTagUpdateSize(HSHM_MCTX, chi::DomainQuery::GetDynamic(),
                                 task->tag_id_, bkt_size_diff,
                                 UpdateSizeMode::kAdd);
    }
    if (task->flags_.Any(HERMES_BLOB_DID_CREATE)) {
      client_.AsyncTagAddBlob(HSHM_MCTX, chi::DomainQuery::GetDynamic(),
                              task->tag_id_, task->blob_id_);
    }

    // Free data
    // HILOG(kInfo, "Completing PUT for {}", task->blob_id_);
    blob_info.UpdateWriteStats();
    IoStat *stat;
    hshm::qtok_t qtok = io_pattern_.push(IoStat{
        IoType::kWrite, task->blob_id_, task->tag_id_, task->data_size_, 0});
    io_pattern_.peek(stat, qtok);
    stat->id_ = qtok.id_;

    HILOG(kDebug,
          "(node {}) Put blob with ID {} data_size={} task_node={} "
          "mod_count={} last_flush={}",
          CHI_CLIENT->node_id_, task->blob_id_, task->data_size_,
          task->task_node_, blob_info.mod_count_, blob_info.last_flush_);
  }
  void MonitorPutBlob(MonitorModeId mode, PutBlobTask *task, RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheWriteRoute<PutBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(PutBlob)

  CHI_BEGIN(GetBlob)
  /** Get a blob */
  void GetBlob(GetBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    // Get blob struct
    if (task->blob_id_.IsNull()) {
      chi::string blob_name(task->blob_name_);
      task->blob_id_ = GetOrCreateBlobId(tls, task->tag_id_,
                                         HashBlobName(task->tag_id_, blob_name),
                                         blob_name, task->flags_);
    }

    // Verify data is non-zero
    if (task->data_size_ == 0) {
      return;
    }

    // Get blob map struct
    BLOB_MAP_T &blob_map = tls.blob_map_;
    if (blob_map.find(task->blob_id_) == blob_map.end()) {
      HELOG(kWarning, "(node {}) Attempting to get non-existent blob {}",
            CHI_CLIENT->node_id_, task->blob_id_);
      return;
    }
    BlobInfo &blob_info = blob_map[task->blob_id_];

    // Stage Blob
    if (blob_info.flags_.Any(HERMES_SHOULD_STAGE) &&
        !blob_info.flags_.Any(HERMES_DID_STAGE_IN)) {
      blob_info.flags_.SetBits(HERMES_DID_STAGE_IN);
      client_.StageIn(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                      task->tag_id_, blob_info.name_, 1);
    }

    // Get blob struct
    chi::ScopedCoRwReadLock blob_info_lock(blob_info.lock_);

    // Read blob from buffers
    std::vector<FullPtr<chi::bdev::ReadTask>> read_tasks;
    read_tasks.reserve(blob_info.buffers_.size());
    HILOG(kDebug,
          "(node={}) Getting blob {} of size {} starting at offset {} "
          "(total_blob_size={}, buffers={})",
          CHI_CLIENT->node_id_, task->blob_id_, task->data_size_,
          task->blob_off_, blob_info.blob_size_, blob_info.buffers_.size());

    DataIterator diter(Slice{task->blob_off_, task->data_size_});
    for (BufferInfo &buf : blob_info.buffers_) {
      diter.Intersect(buf);
      if (diter.DidIntersect()) {
        // HILOG(kInfo,
        //       "(node {}) (alloc={} data={} off={} size={}) (tgt_off={}, "
        //       "tgt_id={})",
        //       CHI_CLIENT->node_id_, task->data_.alloc_id_,
        //       task->data_.off_.load(), buf_off, buf_size, tgt_off, buf.tid_);
        TargetInfo &target = *target_map_[buf.tid_];
        FullPtr<chi::bdev::ReadTask> read_task = target.client_.AsyncRead(
            HSHM_MCTX, target.dom_query_, task->data_ + diter.data_off_,
            diter.tgt_off_, diter.intersect_.size_);
        read_tasks.emplace_back(read_task);
      }
    }
    task->Wait(read_tasks);
    for (FullPtr<chi::bdev::ReadTask> &read_task : read_tasks) {
      CHI_CLIENT->DelTask(HSHM_MCTX, read_task);
    }
    blob_info.UpdateReadStats();
    IoStat *stat;
    hshm::qtok_t qtok = io_pattern_.push(IoStat{
        IoType::kRead, task->blob_id_, task->tag_id_, task->data_size_, 0});
    io_pattern_.peek(stat, qtok);
    stat->id_ = qtok.id_;
    HILOG(kDebug, "Finished getting blob {} of size {} starting at offset {}",
          task->blob_id_, task->data_size_, task->blob_off_);
  }
  void MonitorGetBlob(MonitorModeId mode, GetBlobTask *task, RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<GetBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(GetBlob)

  CHI_BEGIN(AppendBlob)
  /** The AppendBlob method */
  void AppendBlob(AppendBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;

    // Get tag info to get page size and current size
    auto tag_it = tag_map.find(task->tag_id_);
    if (tag_it == tag_map.end()) {
      return;
    }
    TagInfo &tag = tag_it->second;
    size_t page_size = tag.page_size_ > 0
                           ? tag.page_size_
                           : MEGABYTES(1); // Default to 1MB if not set

    // Calculate number of pages needed
    size_t total_pages = (task->data_size_ + page_size - 1) / page_size;

    /// Placement
    size_t data_size = task->data_size_;
    adapter::BlobPlacement placement;
    placement.bucket_off_ = tag.internal_size_;
    placement.page_ = placement.bucket_off_ / page_size;
    placement.blob_off_ = placement.bucket_off_ % page_size;
    placement.blob_size_ = std::min(page_size - placement.blob_off_, data_size);
    hipc::Pointer data(task->data_ + placement.blob_off_);

    // Do partial puts for each page
    for (size_t i = 0; i < total_pages; i++) {
      // Create blob placement and name for this page
      chi::string blob_name = placement.CreateBlobName();

      // Create a PutBlob task for this page
      client_.AsyncPutBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                           task->tag_id_, blob_name, BlobId::GetNull(),
                           placement.blob_off_, placement.blob_size_, data,
                           task->score_, TASK_FIRE_AND_FORGET,
                           task->flags_.bits_);

      // Shift placement
      placement.bucket_off_ += placement.blob_size_;
      placement.page_ = placement.bucket_off_ / page_size;
      placement.blob_off_ = placement.bucket_off_ % page_size;
      placement.blob_size_ =
          std::min(page_size - placement.blob_off_, data_size);
      data += placement.blob_size_;
      data_size -= placement.blob_size_;
    }
  }
  void MonitorAppendBlob(MonitorModeId mode, AppendBlobTask *task,
                         RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
    }
    }
  }
  CHI_END(AppendBlob)

  CHI_BEGIN(TruncateBlob)
  /** Truncate a blob (TODO) */
  void TruncateBlob(TruncateBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
  }
  void MonitorTruncateBlob(MonitorModeId mode, TruncateBlobTask *task,
                           RunContext &rctx) {}
  CHI_END(TruncateBlob)

  CHI_BEGIN(DestroyBlob)
  /** Destroy blob */
  void DestroyBlob(DestroyBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwWriteLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob = it->second;
    // Free blob buffers
    for (BufferInfo &buf : blob.buffers_) {
      TargetInfo &target = *target_map_[buf.tid_];
      target.client_.Free(HSHM_MCTX, target.dom_query_, buf);
      target.stats_->free_ += buf.size_;
    }
    // Remove blob from the tag
    if (!task->flags_.Any(DestroyBlobTask::kKeepInTag)) {
      client_.TagRemoveBlob(HSHM_MCTX, chi::DomainQuery::GetDynamic(),
                            blob.tag_id_, task->blob_id_);
    }
    // Remove the blob from the maps
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    blob_id_map.erase(blob.GetBlobNameWithBucket());
    blob_map.erase(it);
  }
  void MonitorDestroyBlob(MonitorModeId mode, DestroyBlobTask *task,
                          RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheWriteRoute<DestroyBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(DestroyBlob)

  CHI_BEGIN(TagBlob)
  /** Tag a blob */
  void TagBlob(TagBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob = it->second;
    blob.tags_.push_back(task->tag_);
  }
  void MonitorTagBlob(MonitorModeId mode, TagBlobTask *task, RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheWriteRoute<TagBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(TagBlob)

  CHI_BEGIN(BlobHasTag)
  /** Check if blob has a tag */
  void BlobHasTag(BlobHasTagTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    auto it = blob_map.find(task->blob_id_);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob = it->second;
    task->has_tag_ = std::find(blob.tags_.begin(), blob.tags_.end(),
                               task->tag_) != blob.tags_.end();
  }
  void MonitorBlobHasTag(MonitorModeId mode, BlobHasTagTask *task,
                         RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheReadRoute<BlobHasTagTask>(task);
      return;
    }
    }
  }
  CHI_END(BlobHasTag)

  CHI_BEGIN(ReorganizeBlob)
  /** Change blob composition */
  void ReorganizeBlob(ReorganizeBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    BLOB_MAP_T &blob_map = tls.blob_map_;
    // Get blob ID
    chi::string blob_name(task->blob_name_);
    if (task->blob_id_.IsNull()) {
      auto blob_id_map_it = blob_id_map.find(blob_name);
      if (blob_id_map_it == blob_id_map.end()) {
        return;
      }
      task->blob_id_ = blob_id_map_it->second;
    }
    // Get blob struct
    auto blob_map_it = blob_map.find(task->blob_id_);
    if (blob_map_it == blob_map.end()) {
      return;
    }
    BlobInfo &blob_info = blob_map_it->second;
    // Check if it is worth updating the score
    // TODO(llogan)
    // Set the new score
    if (task->is_user_score_) {
      blob_info.user_score_ = task->score_;
      blob_info.score_ = blob_info.user_score_;
    } else {
      blob_info.score_ = task->score_;
    }
    // Get the blob
    FullPtr<char> data =
        CHI_CLIENT->AllocateBuffer(HSHM_MCTX, blob_info.blob_size_);
    client_.GetBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0), task->tag_id_,
                    task->blob_id_, 0, blob_info.blob_size_, data.shm_,
                    0); // OK
    // Put the blob with the new score
    client_.AsyncPutBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                         task->tag_id_, chi::string(""), task->blob_id_, 0,
                         blob_info.blob_size_, data.shm_, blob_info.score_,
                         TASK_FIRE_AND_FORGET | TASK_DATA_OWNER,
                         0); // OK
  }
  void MonitorReorganizeBlob(MonitorModeId mode, ReorganizeBlobTask *task,
                             RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kSchedule: {
      BlobCacheWriteRoute<ReorganizeBlobTask>(task);
      return;
    }
    }
  }
  CHI_END(ReorganizeBlob)

  CHI_BEGIN(ReorganizeNode)
  /** The ReorganizeNode method */
  void ReorganizeNode(ReorganizeNodeTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    BLOB_MAP_T &blob_map = tls.blob_map_;
    for (auto &it : blob_map) {
      BlobInfo &blob_info = it.second;
      // Update blob scores
      //      float new_score = MakeScore(blob_info, now);
      //      blob_info.score_ = new_score;
      //      if (ShouldReorganize<true>(blob_info, new_score,
      //      task->task_node_)) {
      //        Context ctx;
      //        FullPtr<ReorganizeBlobTask> reorg_task =
      //            blob_mdm_.AsyncReorganizeBlob(task->task_node_ + 1,
      //                                          blob_info.tag_id_,
      //                                          chi::string(""),
      //                                          blob_info.blob_id_,
      //                                          new_score, false, ctx,
      //                                          TASK_LOW_LATENCY);
      //        reorg_task->Wait<TASK_YIELD_CO>(task);
      //        CHI_CLIENT->DelTask(HSHM_MCTX, reorg_task);
      //      }
      //      blob_info.access_freq_ = 0;
    }
  }
  void MonitorReorganizeNode(MonitorModeId mode, ReorganizeNodeTask *task,
                             RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
    }
    }
  }
  CHI_END(ReorganizeNode)

  CHI_BEGIN(FlushBlob)
  /** Check if blob needs to be flushed */
  bool _BlobNeedsFlush(BlobInfo &blob_info) {
    return blob_info.flags_.Any(HERMES_SHOULD_STAGE) &&
           blob_info.mod_count_ > blob_info.last_flush_;
  }

  /** Check if any blobs need to be flushed */
  bool _AnyBlobNeedsFlush() {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    BLOB_MAP_T &blob_map = tls.blob_map_;
    for (auto &it : blob_map) {
      BlobInfo &blob_info = it.second;
      if (_BlobNeedsFlush(blob_info)) {
        return true;
      }
    }
    return false;
  }

  /** FlushBlob */
  void _FlushBlob(HermesLane &tls, BlobId blob_id, RunContext &rctx) {
    BLOB_MAP_T &blob_map = tls.blob_map_;
    // Can we find the blob
    auto it = blob_map.find(blob_id);
    if (it == blob_map.end()) {
      return;
    }
    BlobInfo &blob_info = it->second;
    chi::ScopedCoRwReadLock blob_map_lock(blob_info.lock_);
    FlushInfo flush_info;
    flush_info.blob_info_ = &blob_info;
    flush_info.mod_count_ = blob_info.mod_count_;
    // Is the blob already flushed?
    if (!_BlobNeedsFlush(blob_info)) {
      return;
    }
    HILOG(kDebug, "Flushing blob {} (mod_count={}, last_flush={})",
          blob_info.blob_id_, flush_info.mod_count_, blob_info.last_flush_);
    FullPtr<char> data =
        CHI_CLIENT->AllocateBuffer(HSHM_MCTX, blob_info.blob_size_);
    client_.GetBlob(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                    blob_info.tag_id_, blob_info.blob_id_, 0,
                    blob_info.blob_size_, data.shm_, 0); // OK
    adapter::BlobPlacement plcmnt;
    plcmnt.DecodeBlobName(blob_info.name_, 4096);
    HILOG(kDebug, "Flushing blob {} with first entry {}", plcmnt.page_,
          (int)data.ptr_[0]);
    client_.StageOut(HSHM_MCTX, chi::DomainQuery::GetLocalHash(0),
                     blob_info.tag_id_, blob_info.name_, data.shm_,
                     blob_info.blob_size_,
                     TASK_DATA_OWNER); // OK
    HILOG(kDebug, "Finished flushing blob {} with first entry {}", plcmnt.page_,
          (int)data.ptr_[0]);
    blob_info.last_flush_ = flush_info.mod_count_;
  }

  void FlushBlob(FlushBlobTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    _FlushBlob(tls, task->blob_id_, rctx);
  }
  void MonitorFlushBlob(MonitorModeId mode, FlushBlobTask *task,
                        RunContext &rctx) {}
  CHI_END(FlushBlob)

  CHI_BEGIN(FlushData)
  /** Flush blobs back to storage */
  void FlushData(FlushDataTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_ID_MAP_T &blob_id_map = tls.blob_id_map_;
    BLOB_MAP_T &blob_map = tls.blob_map_;
    for (auto &it : blob_map) {
      BlobInfo &blob_info = it.second;
      _FlushBlob(tls, blob_info.blob_id_, rctx);
    }
  }
  void MonitorFlushData(MonitorModeId mode, FlushDataTask *task,
                        RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kFlushWork: {
      bool needs_flush = _AnyBlobNeedsFlush();
      rctx.flush_->count_ += needs_flush;
      if (needs_flush) {
        HILOG(kDebug, "FlushData: needs_flush={}", needs_flush);
      }
    }
    }
  }
  CHI_END(FlushData)

  /** Monitor function used by all metadata poll functions */
  template <typename PollTaskT, typename MD>
  void MonitorPollMetadata(MonitorModeId mode, PollTaskT *task,
                           RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
      std::vector<MD> stats_agg;
      stats_agg.reserve(task->max_count_);
      for (FullPtr<Task> &replica : replicas) {
        PollTaskT *replica_task = replica.Cast<PollTaskT>().ptr_;
        // Merge replicas
        auto stats = replica_task->GetStats();
        size_t append_count = stats.size();
        if (task->max_count_ > 0 && stats_agg.size() < task->max_count_) {
          append_count =
              std::min(append_count, task->max_count_ - stats_agg.size());
        }
        stats_agg.insert(stats_agg.end(), stats.begin(),
                         stats.begin() + append_count);
      }
      task->SetStats(stats_agg);
    }
    }
  }

  CHI_BEGIN(PollBlobMetadata)
  /** Poll blob metadata */
  void PollBlobMetadata(PollBlobMetadataTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock blob_map_lock(tls.blob_map_lock_);
    BLOB_MAP_T &blob_map = tls.blob_map_;
    std::vector<BlobInfo> blob_mdms;
    blob_mdms.reserve(blob_map.size());
    std::string filter = task->filter_.str();
    for (const std::pair<BlobId, BlobInfo> &blob_part : blob_map) {
      const BlobInfo &blob_info = blob_part.second;
      if (!filter.empty()) {
        if (!std::regex_match(blob_info.name_.str(), std::regex(filter))) {
          continue;
        }
      }
      blob_mdms.emplace_back(blob_info);
    }
    task->SetStats(blob_mdms);
  }
  void MonitorPollBlobMetadata(MonitorModeId mode, PollBlobMetadataTask *task,
                               RunContext &rctx) {
    MonitorPollMetadata<PollBlobMetadataTask, BlobInfo>(mode, task, rctx);
  }
  CHI_END(PollBlobMetadata)

  CHI_BEGIN(PollTargetMetadata)
  /** Poll target metadata */
  void PollTargetMetadata(PollTargetMetadataTask *task, RunContext &rctx) {
    std::vector<TargetStats> target_mdms;
    target_mdms.reserve(targets_.size());
    for (const TargetInfo &bdev_client : targets_) {
      bool is_remote = bdev_client.id_.node_id_ != CHI_CLIENT->node_id_;
      if (is_remote) {
        continue;
      }
      TargetStats stats;
      stats.tgt_id_ = bdev_client.id_;
      stats.node_id_ = CHI_CLIENT->node_id_;
      stats.rem_cap_ = bdev_client.stats_->free_;
      stats.max_cap_ = bdev_client.stats_->max_cap_;
      stats.bandwidth_ = bdev_client.stats_->write_bw_;
      stats.latency_ = bdev_client.stats_->write_latency_;
      stats.score_ = bdev_client.score_;
      target_mdms.emplace_back(stats);
    }
    task->SetStats(target_mdms);
  }
  void MonitorPollTargetMetadata(MonitorModeId mode,
                                 PollTargetMetadataTask *task,
                                 RunContext &rctx) {
    MonitorPollMetadata<PollTargetMetadataTask, TargetStats>(mode, task, rctx);
  }
  CHI_END(PollTargetMetadata)

  CHI_BEGIN(PollTagMetadata)
  /** The PollTagMetadata method */
  void PollTagMetadata(PollTagMetadataTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoRwReadLock tag_map_lock(tls.tag_map_lock_);
    TAG_MAP_T &tag_map = tls.tag_map_;
    std::vector<TagInfo> stats;
    std::string filter = task->filter_.str();
    for (auto &it : tag_map) {
      TagInfo &tag = it.second;
      if (!filter.empty()) {
        if (!std::regex_match(tag.name_.str(), std::regex(filter))) {
          continue;
        }
      }
      stats.emplace_back(tag);
    }
    task->SetStats(stats);
  }
  void MonitorPollTagMetadata(MonitorModeId mode, PollTagMetadataTask *task,
                              RunContext &rctx) {
    MonitorPollMetadata<PollTagMetadataTask, TagInfo>(mode, task, rctx);
  }
  CHI_END(PollTagMetadata)

  CHI_BEGIN(PollAccessPattern)
  /** The PollAccessPattern method */
  void PollAccessPattern(PollAccessPatternTask *task, RunContext &rctx) {
    std::vector<IoStat> io_pattern;
    int depth = io_pattern_.GetDepth();
    int qsize = io_pattern_.GetSize();
    int iter_size = std::min(depth, qsize);
    io_pattern.reserve(iter_size);
    for (int i = 0; i < iter_size; ++i) {
      IoStat *stat;
      hshm::qtok_t qtok = io_pattern_.peek(stat, i);
      if (task->last_access_ > 0 && stat->id_ < task->last_access_) {
        continue;
      }
      io_pattern.emplace_back(*stat);
    }
    std::sort(io_pattern.begin(), io_pattern.end(),
              [](const IoStat &a, const IoStat &b) { return a.id_ < b.id_; });
    task->io_pattern_ = io_pattern;
    if (!io_pattern.empty()) {
      task->last_access_ = io_pattern.back().id_;
    }
  }
  void MonitorPollAccessPattern(MonitorModeId mode, PollAccessPatternTask *task,
                                RunContext &rctx) {}
  CHI_END(PollAccessPattern)

  /**
   * ========================================
   * STAGING Tasks
   * ========================================
   * */

  CHI_BEGIN(RegisterStager)
  /** The RegisterStager method */
  void RegisterStager(RegisterStagerTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoMutex stager_map_lock(tls.stager_map_lock_);
    STAGER_MAP_T &stager_map = tls.stager_map_;
    std::string tag_name = task->tag_name_.str();
    std::string params = task->params_.str();
    HILOG(kDebug, "Registering stager {}: {}", task->bkt_id_, tag_name);
    std::shared_ptr<AbstractStager> stager =
        StagerFactory::Get(tag_name, params);
    stager->RegisterStager(HSHM_MCTX, task->tag_name_.str(),
                           task->params_.str());
    stager_map.emplace(task->bkt_id_, std::move(stager));
    HILOG(kDebug, "Finished registering stager {}: {}", task->bkt_id_,
          tag_name);
  }
  void MonitorRegisterStager(MonitorModeId mode, RegisterStagerTask *task,
                             RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
    }
    }
  }
  CHI_END(RegisterStager)

  CHI_BEGIN(UnregisterStager)
  /** The UnregisterStager method */
  void UnregisterStager(UnregisterStagerTask *task, RunContext &rctx) {
    HILOG(kDebug, "Unregistering stager {}", task->bkt_id_);
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoMutex stager_map_lock(tls.stager_map_lock_);
    STAGER_MAP_T &stager_map = tls.stager_map_;
    if (stager_map.find(task->bkt_id_) == stager_map.end()) {
      return;
    }
    stager_map.erase(task->bkt_id_);
  }
  void MonitorUnregisterStager(MonitorModeId mode, UnregisterStagerTask *task,
                               RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
    }
    }
  }
  CHI_END(UnregisterStager)

  CHI_BEGIN(StageIn)
  /** The StageIn method */
  void StageIn(StageInTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoMutex stager_map_lock(tls.stager_map_lock_);
    STAGER_MAP_T &stager_map = tls.stager_map_;
    STAGER_MAP_T::iterator it = stager_map.find(task->bkt_id_);
    if (it == stager_map.end()) {
      // HELOG(kError, "Could not find stager for bucket: {}", task->bkt_id_);
      // TODO(llogan): Probably should add back...
      // task->SetModuleComplete();
      return;
    }
    std::shared_ptr<AbstractStager> &stager = it->second;
    stager->StageIn(HSHM_MCTX, client_, task->bkt_id_, task->blob_name_.str(),
                    task->score_);
  }
  void MonitorStageIn(MonitorModeId mode, StageInTask *task, RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
    }
    }
  }
  CHI_END(StageIn)

  CHI_BEGIN(StageOut)
  /** The StageOut method */
  void StageOut(StageOutTask *task, RunContext &rctx) {
    HermesLane &tls = tls_[CHI_CUR_LANE->lane_id_];
    chi::ScopedCoMutex stager_map_lock(tls.stager_map_lock_);
    STAGER_MAP_T &stager_map = tls.stager_map_;
    STAGER_MAP_T::iterator it = stager_map.find(task->bkt_id_);
    if (it == stager_map.end()) {
      HELOG(kError, "Could not find stager for bucket: {}", task->bkt_id_);
      return;
    }
    std::shared_ptr<AbstractStager> &stager = it->second;
    stager->StageOut(HSHM_MCTX, client_, task->bkt_id_, task->blob_name_.str(),
                     task->data_, task->data_size_);
  }
  void MonitorStageOut(MonitorModeId mode, StageOutTask *task,
                       RunContext &rctx) {
    switch (mode) {
    case MonitorMode::kReplicaAgg: {
      std::vector<FullPtr<Task>> &replicas = *rctx.replicas_;
    }
    }
  }
  CHI_END(StageOut)

  CHI_AUTOGEN_METHODS
public:
#include "hermes_core/hermes_core_lib_exec.h"
};

} // namespace hermes

CHI_TASK_CC(hermes::Server, "hermes_core");
