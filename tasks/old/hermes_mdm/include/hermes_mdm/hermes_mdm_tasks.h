//
// Created by lukemartinlogan on 8/14/23.
//

#ifndef HRUN_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_TASKS_H_
#define HRUN_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_TASKS_H_

#include "chimaera/api/chimaera_client.h"
#include "chimaera/module_registry/task_lib.h"
#include "chimaera_admin/chimaera_admin.h"
#include "chimaera/queue_manager/queue_manager_client.h"
#include "hermes/hermes_types.h"
#include "bdev/bdev.h"
#include "proc_queue/proc_queue.h"

namespace hermes::mdm {

#include "hermes_core_methods.h"
#include "chimaera/chimaera_namespace.h"

/**
 * A task to create hermes_core
 * */
using chi::Admin::CreateTaskStateTask;
struct ConstructTask : public CreateTaskStateTask {
  IN chi::string server_config_path_;

  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  ConstructTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : CreateTaskStateTask(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  ConstructTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
                const TaskNode &task_node,
                const DomainQuery &dom_query,
                const std::string &state_name,
                const PoolId &id,
                const std::vector<PriorityInfo> &queue_info,
                const std::string &server_config_path = "")
      : CreateTaskStateTask(alloc, task_node, domain_id, state_name,
                            "hermes_core", id, queue_info) {
    // Custom params
    HSHM_MAKE_AR(server_config_path_, alloc, server_config_path);
    std::stringstream ss;
    cereal::BinaryOutputArchive ar(ss);
    ar(server_config_path_);
    std::string data = ss.str();
    *custom_ = data;
  }

  void Deserialize() {
    std::string data = custom_->str();
    std::stringstream ss(data);
    cereal::BinaryInputArchive ar(ss);
    ar(server_config_path_);
  }
};

/** A task to destroy hermes_core */
using chi::Admin::DestroyTaskStateTask;
struct DestructTask : public DestroyTaskStateTask {
  /** SHM default constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc) : DestroyTaskStateTask(alloc) {}

  /** Emplace constructor */
  HSHM_ALWAYS_INLINE explicit
  DestructTask(const hipc::CtxAllocator<CHI_ALLOC_T> &alloc,
               const TaskNode &task_node,
               const DomainQuery &dom_query,
               const PoolId &pool_id)
      : DestroyTaskStateTask(alloc, task_node, domain_id, pool_id) {}

  /** Create group */
  HSHM_ALWAYS_INLINE
  u32 GetGroup(chi::charbuf &group) {
    return TASK_UNORDERED;
  }
};

}  // namespace hermes::mdm

#endif  // HRUN_TASKS_HERMES_MDM_INCLUDE_HERMES_MDM_HERMES_MDM_TASKS_H_