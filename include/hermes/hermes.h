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

#ifndef HRUN_TASKS_HERMES_INCLUDE_HERMES_HERMES_H_
#define HRUN_TASKS_HERMES_INCLUDE_HERMES_HERMES_H_

#include "hermes/bucket.h"
#include "hermes/config_manager.h"
#ifdef CHIMAERA_RUNTIME
#include "hermes/hermes_run_types.h"
#endif

namespace hermes {

class Hermes {
 public:
  /** Init hermes client */
  void ClientInit() { HERMES_CONF->ClientInit(); }

  /** Init hermes server */
  void ServerInit() { HERMES_CONF->ServerInit(); }

  /** Check if initialized */
  bool IsInitialized() { return HERMES_CONF->is_initialized_; }

  /** Get a bucket */
  Bucket GetBucket(const std::string &name) { return hermes::Bucket(name); }

  /** Collects blob metadata */
  std::vector<BlobInfo> PollBlobMetadata(const std::string &filter,
                                         int max_count) {
    return HERMES_CONF->mdm_.PollBlobMetadata(
        HSHM_MCTX, chi::DomainQuery::GetGlobalBcast(), filter, max_count);
  }

  /** Collects tag metadata */
  std::vector<TagInfo> PollTagMetadata(const std::string &filter,
                                       int max_count) {
    return HERMES_CONF->mdm_.PollTagMetadata(
        HSHM_MCTX, chi::DomainQuery::GetGlobalBcast(), filter, max_count);
  }

  /** Collects target metadata */
  std::vector<TargetStats> PollTargetMetadata(const std::string &filter,
                                              int max_count) {
    return HERMES_CONF->mdm_.PollTargetMetadata(
        HSHM_MCTX, chi::DomainQuery::GetGlobalBcast(), filter, max_count);
  }

  /**
   * Poll the access pattern log.
   * @param last_access The id of the last element accessed.
   * @return A vector of IoStat objects sorted by id.
   */
  std::vector<IoStat> PollAccessPattern(int last_access) {
    return HERMES_CONF->mdm_.PollAccessPattern(
        HSHM_MCTX,
        chi::DomainQuery::GetDirectHash(chi::SubDomainId::kLocalContainers, 0),
        last_access);
  }

  /** Get tag id */
  TagId GetTagId(const hipc::MemContext &mctx, const std::string &tag_name) {
    return HERMES_CONF->mdm_.GetTagId(mctx, DomainQuery::GetDynamic(),
                                      chi::string(tag_name));
  }

  /** Clear all data from hermes */
  void Clear() {
    // TODO(llogan)
  }
};

#define HERMES hshm::Singleton<::hermes::Hermes>::GetInstance()

}  // namespace hermes

#endif  // HRUN_TASKS_HERMES_INCLUDE_HERMES_HERMES_H_
