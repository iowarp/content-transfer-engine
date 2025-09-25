#include "hermes_shm/util/logging.h"
#include <algorithm>
#include <wrp_cte/core/core_config.h>
#include <wrp_cte/core/core_dpe.h>
#include <wrp_cte/core/core_runtime.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace wrp_cte::core {

// No more static member definitions - using instance-based locking

void Runtime::Create(hipc::FullPtr<CreateTask> task, chi::RunContext &ctx) {
  // Initialize the container with pool information and domain query
  chi::Container::Init(task->pool_id_, task->pool_query_);

  // Initialize lock vectors for concurrent access
  target_locks_.reserve(kMaxLocks);
  tag_locks_.reserve(kMaxLocks);
  for (size_t i = 0; i < kMaxLocks; ++i) {
    target_locks_.emplace_back(std::make_unique<chi::CoRwLock>());
    tag_locks_.emplace_back(std::make_unique<chi::CoRwLock>());
  }

  // Initialize telemetry ring buffer
  telemetry_log_ = hipc::circular_mpsc_queue<CteTelemetry>(main_allocator_,
                                                           kTelemetryRingSize);

  // Initialize atomic counters
  next_tag_id_minor_ = 1;
  next_blob_id_minor_ = 1;
  telemetry_counter_ = 0;

  // Initialize configuration manager with allocator
  auto *config_manager = &ConfigManager::GetInstance();
  config_manager->Initialize(main_allocator_);

  // Load configuration from file if provided
  auto params = task->GetParams(main_allocator_);
  std::string config_path = params.config_file_path_.str();

  bool config_loaded = false;
  if (!config_path.empty()) {
    config_loaded = config_manager->LoadConfig(config_path);
  } else {
    // Try loading from environment variable
    config_loaded = config_manager->LoadConfigFromEnvironment();
  }

  // Get configuration (will use defaults if loading failed)
  const Config &config = config_manager->GetConfig();

  // Store storage configuration in runtime
  storage_devices_ = config.storage_.devices_;

  // Initialize the client so we can use the CTE client API
  InitClient(pool_id_);

  // Register targets for each configured storage device during initialization
  if (!storage_devices_.empty()) {
    HILOG(kInfo, "Registering targets for storage devices:");

    for (size_t i = 0; i < storage_devices_.size(); ++i) {
      const auto &device = storage_devices_[i];
      std::string target_name = "storage_device_" + std::to_string(i);

      // TODO: Implement target registration when bdev module is available
      // For now, simulate successful target registration
      chi::u32 result = 0; // Success

      // Store device information for later use
      (void)device; // Suppress unused variable warning

      if (result == 0) {
        HILOG(kInfo, "  - Registered target '{}': {} ({}, {} bytes)",
              target_name, device.path_, device.bdev_type_,
              device.capacity_limit_);
      } else {
        HILOG(kInfo, "  - Failed to register target '{}' (error code: {})",
              target_name, result);
      }
    }
  } else {
    HILOG(kInfo, "Warning: No storage devices configured");
  }

  // Create local queues using configuration settings
  CreateLocalQueue(kTargetManagementQueue,
                   config.target_management_queue_.lane_count_,
                   config.target_management_queue_.priority_);
  CreateLocalQueue(kTagManagementQueue,
                   config.tag_management_queue_.lane_count_,
                   config.tag_management_queue_.priority_);
  CreateLocalQueue(kBlobOperationsQueue,
                   config.blob_operations_queue_.lane_count_,
                   config.blob_operations_queue_.priority_);
  CreateLocalQueue(kStatsQueue, config.stats_queue_.lane_count_,
                   config.stats_queue_.priority_);

  // Initialize atomic counters
  next_tag_id_minor_.store(1);  // Start tag ID minors from 1
  next_blob_id_minor_.store(1); // Start blob ID minors from 1

  HILOG(kInfo,
        "CTE Core container created and initialized for pool: {} (ID: {})",
        pool_name_, task->pool_id_);

  HILOG(kInfo, "Configuration: worker_count={}, max_targets={}",
        config.worker_count_, config.targets_.max_targets_);

  if (config_loaded) {
    if (!config_path.empty()) {
      HILOG(kInfo, "Configuration loaded from file: {}", config_path);
    } else {
      HILOG(kInfo, "Configuration loaded from environment");
    }
  } else {
    HILOG(kInfo, "Using default configuration");
  }
}

void Runtime::MonitorCreate(chi::MonitorModeId mode,
                            hipc::FullPtr<CreateTask> task,
                            chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to target management queue for container initialization
    auto lane_ptr = GetLaneFullPtr(kTargetManagementQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule: {
    // Optional: Global coordination for distributed setup
    break;
  }
  case chi::MonitorModeId::kEstLoad: {
    // Estimate container creation time
    ctx.estimated_completion_time_us = 5000.0; // 5ms for container creation
    break;
  }
  }
}

void Runtime::Destroy(hipc::FullPtr<DestroyTask> task, chi::RunContext &ctx) {
  try {
    // Clear all registered targets and their associated data
    registered_targets_.clear();
    target_name_to_id_.clear();

    // Clear tag and blob management structures
    tag_name_to_id_.clear();
    tag_id_to_info_.clear();
    tag_blob_name_to_id_.clear();
    blob_id_to_info_.clear();

    // Reset atomic counters
    next_tag_id_minor_.store(1);
    next_blob_id_minor_.store(1);

    // Clear storage device configuration
    storage_devices_.clear();

    // Clear lock vectors
    target_locks_.clear();
    tag_locks_.clear();

    // Set success status
    task->return_code_ = 0;

  } catch (const std::exception &e) {
    task->return_code_ = 1;
  }
}

void Runtime::MonitorDestroy(chi::MonitorModeId mode,
                             hipc::FullPtr<DestroyTask> task,
                             chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to target management queue for container cleanup
    auto lane_ptr = GetLaneFullPtr(kTargetManagementQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule: {
    // Optional: Global coordination for distributed cleanup
    break;
  }
  case chi::MonitorModeId::kEstLoad: {
    // Estimate container destruction time
    ctx.estimated_completion_time_us = 1000.0; // 1ms for container cleanup
    break;
  }
  }
}

void Runtime::RegisterTarget(hipc::FullPtr<RegisterTargetTask> task,
                             chi::RunContext &ctx) {
  try {
    std::string target_name = task->target_name_.str();
    chimaera::bdev::BdevType bdev_type = task->bdev_type_;
    chi::u64 total_size = task->total_size_;

    // Create bdev client and container first to get the TargetId (pool_id)
    chimaera::bdev::Client bdev_client;
    std::string bdev_pool_name =
        target_name; // Use target_name as the bdev pool name

    // Create the bdev container using the client
    chi::PoolQuery pool_query = chi::PoolQuery::Local();
    bdev_client.Create(hipc::MemContext(), pool_query, target_name, bdev_type,
                       total_size);

    // Check if creation was successful
    if (bdev_client.return_code_ != 0) {
      task->result_code_ = 1;
      return;
    }

    // Get the TargetId (bdev_client's pool_id) for indexing
    chi::PoolId target_id = bdev_client.pool_id_;

    // Check if target is already registered using TargetId
    size_t lock_index = GetTargetLockIndex(target_id);
    {
      chi::ScopedCoRwReadLock read_lock(*target_locks_[lock_index]);
      if (registered_targets_.find(target_id) != registered_targets_.end()) {
        task->result_code_ = 1;
        return;
      }
    }

    // Get actual statistics from bdev using GetStats method
    chi::u64 remaining_size;
    chimaera::bdev::PerfMetrics perf_metrics =
        bdev_client.GetStats(hipc::MemContext(), remaining_size);

    // Create target info with bdev client and performance stats
    TargetInfo target_info(main_allocator_);
    target_info.target_name_ = target_name;
    target_info.bdev_pool_name_ = bdev_pool_name;
    target_info.bdev_client_ = std::move(bdev_client);
    target_info.bytes_read_ = 0;
    target_info.bytes_written_ = 0;
    target_info.ops_read_ = 0;
    target_info.ops_written_ = 0;
    target_info.target_score_ =
        0.0f; // Will be calculated based on performance metrics
    target_info.remaining_space_ =
        remaining_size; // Use actual remaining space from bdev
    target_info.perf_metrics_ =
        perf_metrics; // Store the entire PerfMetrics structure

    // Register the target using TargetId as key
    {
      chi::ScopedCoRwWriteLock write_lock(*target_locks_[lock_index]);
      registered_targets_[target_id] = target_info;
      target_name_to_id_[target_name] = target_id; // Maintain reverse lookup
    }

    task->result_code_ = 0; // Success
    HILOG(kInfo,
          "Target '{}' registered with bdev pool: {} (type={}, path={}, "
          "size={}, remaining={})",
          target_name, bdev_pool_name, static_cast<int>(bdev_type), target_name,
          total_size, remaining_size);
    HILOG(kInfo,
          "  Initial statistics: read_bw={} MB/s, write_bw={} MB/s, "
          "avg_latency={} μs, iops={}",
          perf_metrics.read_bandwidth_mbps_, perf_metrics.write_bandwidth_mbps_,
          (target_info.perf_metrics_.read_latency_us_ +
           target_info.perf_metrics_.write_latency_us_) /
              2.0,
          perf_metrics.iops_);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorRegisterTarget(chi::MonitorModeId mode,
                                    hipc::FullPtr<RegisterTargetTask> task,
                                    chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to target management queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kTargetManagementQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    // Global coordination for distributed target registration
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate execution time for target registration
    ctx.estimated_completion_time_us = 10000.0; // 10ms for bdev creation
    break;
  }
}

void Runtime::UnregisterTarget(hipc::FullPtr<UnregisterTargetTask> task,
                               chi::RunContext &ctx) {
  try {
    std::string target_name = task->target_name_.str();

    // Look up TargetId from target_name
    auto name_it = target_name_to_id_.find(target_name);
    if (name_it == target_name_to_id_.end()) {
      task->result_code_ = 1;
      return;
    }

    chi::PoolId target_id = name_it->second;

    // Check if target exists and remove it (don't destroy bdev container)
    size_t lock_index = GetTargetLockIndex(target_id);
    {
      chi::ScopedCoRwWriteLock write_lock(*target_locks_[lock_index]);
      auto it = registered_targets_.find(target_id);
      if (it == registered_targets_.end()) {
        task->result_code_ = 1;
        return;
      }

      registered_targets_.erase(it);
      target_name_to_id_.erase(target_name); // Remove reverse lookup
    }

    task->result_code_ = 0; // Success
    HILOG(kInfo, "Target '{}' unregistered", target_name);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorUnregisterTarget(chi::MonitorModeId mode,
                                      hipc::FullPtr<UnregisterTargetTask> task,
                                      chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to target management queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kTargetManagementQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    // Global coordination for distributed target management
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate execution time for target unregistration
    ctx.estimated_completion_time_us = 1000.0; // 1ms for unlinking
    break;
  }
}

void Runtime::ListTargets(hipc::FullPtr<ListTargetsTask> task,
                          chi::RunContext &ctx) {
  try {
    // Clear the output vector and populate with current targets
    task->targets_.clear();

    // Use a single lock based on hash of operation type for listing
    size_t lock_index =
        std::hash<std::string>{}("list_targets") % target_locks_.size();
    chi::ScopedCoRwReadLock read_lock(*target_locks_[lock_index]);

    // Populate target list while lock is held
    task->targets_.reserve(registered_targets_.size());
    for (const auto &pair : registered_targets_) {
      const TargetInfo &target_info = pair.second;
      task->targets_.emplace_back(target_info);
    }

    task->result_code_ = 0; // Success

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorListTargets(chi::MonitorModeId mode,
                                 hipc::FullPtr<ListTargetsTask> task,
                                 chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to target management queue (any lane is fine for read operations)
    auto lane_ptr = GetLaneFullPtr(kTargetManagementQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    // Global coordination for distributed target listing
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate execution time for target listing
    ctx.estimated_completion_time_us = 500.0; // 0.5ms for listing
    break;
  }
}

void Runtime::StatTargets(hipc::FullPtr<StatTargetsTask> task,
                          chi::RunContext &ctx) {
  try {
    // Update performance stats for all registered targets
    // Use a single lock based on hash of operation type for stats
    size_t lock_index =
        std::hash<std::string>{}("stat_targets") % target_locks_.size();
    chi::ScopedCoRwWriteLock write_lock(*target_locks_[lock_index]);

    // Update stats for all targets while lock is held
    for (auto &pair : registered_targets_) {
      const chi::PoolId &target_id = pair.first;
      TargetInfo &target_info = pair.second;
      UpdateTargetStats(target_id, target_info);
    }

    task->result_code_ = 0; // Success

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorStatTargets(chi::MonitorModeId mode,
                                 hipc::FullPtr<StatTargetsTask> task,
                                 chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to stats queue for performance monitoring
    auto lane_ptr = GetLaneFullPtr(kStatsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    // Global coordination for distributed stats collection
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate execution time for stats update
    ctx.estimated_completion_time_us = 2000.0; // 2ms for stats polling
    break;
  }
}

template <typename CreateParamsT>
void Runtime::GetOrCreateTag(
    hipc::FullPtr<GetOrCreateTagTask<CreateParamsT>> task,
    chi::RunContext &ctx) {
  try {
    std::string tag_name = task->tag_name_.str();
    TagId preferred_id = task->tag_id_;

    // Get or assign tag ID
    TagId tag_id = GetOrAssignTagId(tag_name, preferred_id);
    task->tag_id_ = tag_id;

    // Populate tag info and update timestamps
    size_t tag_lock_index = GetTagLockIndex(tag_name);
    auto now = std::chrono::steady_clock::now();
    {
      chi::ScopedCoRwReadLock read_lock(*tag_locks_[tag_lock_index]);
      auto it = tag_id_to_info_.find(tag_id);
      if (it != tag_id_to_info_.end()) {
        // Update read timestamp and get info for telemetry
        const_cast<TagInfo &>(it->second).last_read_ = now;
        task->tag_info_ = it->second;

        // Log telemetry for GetOrCreateTag operation
        LogTelemetry(CteOp::kGetOrCreateTag, 0, 0, BlobId::GetNull(), tag_id,
                     it->second.last_modified_, now);
      }
    }

    task->result_code_ = 0; // Success

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

template <typename CreateParamsT>
void Runtime::MonitorGetOrCreateTag(
    chi::MonitorModeId mode,
    hipc::FullPtr<GetOrCreateTagTask<CreateParamsT>> task,
    chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to tag management queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kTagManagementQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    // Global coordination for distributed tag management
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate execution time for tag operations
    ctx.estimated_completion_time_us = 1000.0; // 1ms for tag lookup/creation
    break;
  }
}

void Runtime::PutBlob(hipc::FullPtr<PutBlobTask> task, chi::RunContext &ctx) {
  try {
    // Extract input parameters
    TagId tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    BlobId blob_id = task->blob_id_;
    chi::u64 offset = task->offset_;
    chi::u64 size = task->size_;
    hipc::Pointer blob_data = task->blob_data_;
    float blob_score = task->score_;
    chi::u32 flags = task->flags_;

    // Suppress unused variable warning for flags - may be used in future
    (void)flags;

    // Validate input parameters
    if (size == 0 || blob_data.IsNull()) {
      task->result_code_ = 1;
      return;
    }

    // Validate that either blob_id or blob_name is provided
    bool blob_id_provided = (blob_id.major_ != 0 || blob_id.minor_ != 0);
    bool blob_name_provided = !blob_name.empty();

    if (!blob_id_provided && !blob_name_provided) {
      task->result_code_ = 1;
      return;
    }

    // Step 1: Check if blob exists
    BlobId found_blob_id;
    BlobInfo *blob_info_ptr =
        CheckBlobExists(blob_id, blob_name, tag_id, found_blob_id);
    bool blob_found = (blob_info_ptr != nullptr);

    // Step 2: Create blob if it doesn't exist
    if (!blob_found) {
      blob_info_ptr =
          CreateNewBlob(blob_name, tag_id, blob_score, found_blob_id);
      if (blob_info_ptr == nullptr) {
        task->result_code_ = 1;
        return;
      }
      task->blob_id_ = found_blob_id;
    }

    // Step 2.5: Track blob size before modification for tag total_size_
    // accounting
    chi::u64 old_blob_size = blob_info_ptr->GetTotalSize();

    // Step 3: Allocate additional space if needed for blob extension
    chi::u32 allocation_result =
        AllocateNewData(*blob_info_ptr, offset, size, blob_score);
    if (allocation_result != 0) {
      task->result_code_ = allocation_result;
      return;
    }

    // Step 4: Write data to blob blocks
    chi::u32 write_result =
        ModifyExistingData(blob_info_ptr->blocks_, blob_data, size, offset);
    if (write_result != 0) {
      task->result_code_ = write_result;
      return;
    }

    // Step 5: Update tag total_size_ with blob size change
    chi::u64 new_blob_size = blob_info_ptr->GetTotalSize();
    chi::i64 size_change = static_cast<chi::i64>(new_blob_size) -
                           static_cast<chi::i64>(old_blob_size);

    // Step 6: Update timestamps and log telemetry
    auto now = std::chrono::steady_clock::now();
    blob_info_ptr->last_modified_ = now;

    // Update tag's total_size_ and timestamps
    auto tag_info_it = tag_id_to_info_.find(tag_id);
    if (tag_info_it != tag_id_to_info_.end()) {
      TagInfo &tag_info = tag_info_it->second;
      tag_info.last_modified_ = now;

      // Use signed arithmetic to handle size decreases
      if (size_change >= 0) {
        tag_info.total_size_ += static_cast<size_t>(size_change);
      } else {
        // Prevent underflow for size decreases
        size_t decrease = static_cast<size_t>(-size_change);
        if (decrease <= tag_info.total_size_) {
          tag_info.total_size_ -= decrease;
        } else {
          tag_info.total_size_ = 0; // Clamp to 0 if we would underflow
        }
      }
    }

    // Log telemetry for PutBlob operation
    LogTelemetry(CteOp::kPutBlob, offset, size, found_blob_id, tag_id, now,
                 blob_info_ptr->last_read_);

    // Success - log operation details
    task->result_code_ = 0;
    HILOG(kInfo,
          "PutBlob successful: blob_id={},{}, name={}, offset={}, size={}, "
          "score={}, blocks={}, tag_total_size={}",
          found_blob_id.major_, found_blob_id.minor_, blob_name, offset, size,
          blob_score, blob_info_ptr->blocks_.size(),
          tag_info_it != tag_id_to_info_.end() ? tag_info_it->second.total_size_
                                               : 0);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorPutBlob(chi::MonitorModeId mode,
                             hipc::FullPtr<PutBlobTask> task,
                             chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate based on blob size
    ctx.estimated_completion_time_us = task->size_ / 1000.0; // 1 us per KB
    break;
  }
}

void Runtime::GetBlob(hipc::FullPtr<GetBlobTask> task, chi::RunContext &ctx) {
  try {
    // Extract input parameters
    TagId tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    BlobId blob_id = task->blob_id_;
    chi::u64 offset = task->offset_;
    chi::u64 size = task->size_;
    chi::u32 flags = task->flags_;

    // Suppress unused variable warning for flags - may be used in future
    (void)flags;

    // Validate input parameters
    if (size == 0) {
      task->result_code_ = 1;
      return;
    }

    // Check if blob name is non-empty and exists. If it does, then check if the
    // ID exists. If it doesn't, error.
    bool blob_id_provided = (blob_id.major_ != 0 || blob_id.minor_ != 0);
    bool blob_name_provided = !blob_name.empty();

    if (!blob_id_provided && !blob_name_provided) {
      task->result_code_ = 1;
      return;
    }

    // Step 1: Check if blob exists
    BlobId found_blob_id;
    BlobInfo *blob_info_ptr =
        CheckBlobExists(blob_id, blob_name, tag_id, found_blob_id);

    // If blob doesn't exist, error
    if (blob_info_ptr == nullptr) {
      task->result_code_ = 1;
      return;
    }

    // Check if blob id is non-null and exists. If it doesn't, error.
    if (blob_id_provided && (found_blob_id.major_ != blob_id.major_ ||
                             found_blob_id.minor_ != blob_id.minor_)) {
      task->result_code_ = 1;
      return;
    }

    // Use the pre-provided data pointer from the task
    hipc::Pointer blob_data_ptr = task->blob_data_;

    // Step 2: Read data from blob blocks using the same clamping and offset
    // calculation logic as ModifyExistingData
    chi::u32 read_result =
        ReadData(blob_info_ptr->blocks_, blob_data_ptr, size, offset);
    if (read_result != 0) {
      task->result_code_ = read_result;
      return;
    }

    // Step 3: Update timestamps and log telemetry
    auto now = std::chrono::steady_clock::now();
    blob_info_ptr->last_read_ = now;

    // Log telemetry for GetBlob operation
    LogTelemetry(CteOp::kGetBlob, offset, size, found_blob_id, tag_id,
                 blob_info_ptr->last_modified_, now);

    // Success - log operation details
    task->result_code_ = 0;
    HILOG(kInfo,
          "GetBlob successful: blob_id={},{}, name={}, offset={}, size={}, "
          "blocks={}",
          found_blob_id.major_, found_blob_id.minor_, blob_name, offset, size,
          blob_info_ptr->blocks_.size());

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorGetBlob(chi::MonitorModeId mode,
                             hipc::FullPtr<GetBlobTask> task,
                             chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate based on blob size
    ctx.estimated_completion_time_us =
        task->size_ / 2000.0; // 0.5 us per KB (read is faster)
    break;
  }
}

void Runtime::ReorganizeBlob(hipc::FullPtr<ReorganizeBlobTask> task,
                             chi::RunContext &ctx) {
  // Unimplemented for now
  task->result_code_ = 2; // Not implemented
}

void Runtime::MonitorReorganizeBlob(chi::MonitorModeId mode,
                                    hipc::FullPtr<ReorganizeBlobTask> task,
                                    chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate for score update
    ctx.estimated_completion_time_us = 100.0; // 0.1ms for score update
    break;
  }
}

void Runtime::DelBlob(hipc::FullPtr<DelBlobTask> task, chi::RunContext &ctx) {
  try {
    // Extract input parameters
    TagId tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    BlobId blob_id = task->blob_id_;

    // Validate that either blob_id or blob_name is provided
    bool blob_id_provided = (blob_id.major_ != 0 || blob_id.minor_ != 0);
    bool blob_name_provided = !blob_name.empty();

    if (!blob_id_provided && !blob_name_provided) {
      task->result_code_ = 1;
      return;
    }

    // Step 1: Check if blob exists
    BlobId found_blob_id;
    BlobInfo *blob_info_ptr =
        CheckBlobExists(blob_id, blob_name, tag_id, found_blob_id);

    if (blob_info_ptr == nullptr) {
      task->result_code_ = 1; // Blob not found
      return;
    }

    // Step 2: Get blob size before deletion for tag size accounting
    chi::u64 blob_size = blob_info_ptr->GetTotalSize();

    // Step 2.5: Free all blocks back to their targets before removing blob
    chi::u32 free_result = FreeAllBlobBlocks(*blob_info_ptr);
    if (free_result != 0) {
      HILOG(kWarning,
            "Failed to free some blocks for blob_id={},{}, continuing with "
            "deletion",
            found_blob_id.major_, found_blob_id.minor_);
      // Continue with deletion even if freeing fails to avoid orphaned blob
      // entries
    }

    // Step 3: Remove blob from tag's blob set
    auto tag_info_it = tag_id_to_info_.find(tag_id);
    if (tag_info_it != tag_id_to_info_.end()) {
      TagInfo &tag_info = tag_info_it->second;
      tag_info.blob_ids_.erase(found_blob_id);

      // Step 4: Decrement tag's total_size_
      if (blob_size <= tag_info.total_size_) {
        tag_info.total_size_ -= blob_size;
      } else {
        tag_info.total_size_ = 0; // Clamp to 0 if we would underflow
      }
    }

    // Step 5: Remove blob from blob_id_to_info_ map
    blob_id_to_info_.erase(found_blob_id);

    // Step 6: Remove blob name mapping if it exists
    if (blob_name_provided) {
      std::string compound_key = std::to_string(tag_id.major_) + "." +
                                 std::to_string(tag_id.minor_) + "." +
                                 blob_name;
      tag_blob_name_to_id_.erase(compound_key);
    }

    // Step 7: Log telemetry for DelBlob operation
    auto now = std::chrono::steady_clock::now();
    LogTelemetry(CteOp::kDelBlob, 0, blob_size, found_blob_id, tag_id, now,
                 now);

    // Success
    task->result_code_ = 0;
    HILOG(kInfo, "DelBlob successful: blob_id={},{}, name={}, blob_size={}",
          found_blob_id.major_, found_blob_id.minor_, blob_name, blob_size);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorDelBlob(chi::MonitorModeId mode,
                             hipc::FullPtr<DelBlobTask> task,
                             chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate for blob deletion
    ctx.estimated_completion_time_us = 50.0; // 0.05ms for deletion
    break;
  }
}

void Runtime::DelTag(hipc::FullPtr<DelTagTask> task, chi::RunContext &ctx) {
  try {
    TagId tag_id = task->tag_id_;
    std::string tag_name = task->tag_name_.str();

    // Step 1: Resolve tag ID if tag name was provided instead
    if (tag_id.IsNull() && !tag_name.empty()) {
      // Look up tag ID by name
      auto name_to_id_it = tag_name_to_id_.find(tag_name);
      if (name_to_id_it == tag_name_to_id_.end()) {
        task->result_code_ = 1; // Tag not found by name
        return;
      }
      tag_id = name_to_id_it->second;
      task->tag_id_ = tag_id; // Update task with resolved tag ID
    } else if (tag_id.IsNull() && tag_name.empty()) {
      task->result_code_ = 1; // Neither tag ID nor tag name provided
      return;
    }

    // Step 2: Find the tag by ID
    auto tag_info_it = tag_id_to_info_.find(tag_id);
    if (tag_info_it == tag_id_to_info_.end()) {
      task->result_code_ = 1; // Tag not found by ID
      return;
    }

    TagInfo &tag_info = tag_info_it->second;

    // Step 3: Delete all blobs in this tag using client AsyncDelBlob to
    // properly clean up blocks
    auto *cte_client = WRP_CTE_CLIENT;

    // Process blobs in batches to limit concurrent async tasks
    constexpr size_t kMaxConcurrentDelBlobTasks = 32;
    std::vector<hipc::FullPtr<DelBlobTask>> async_tasks;
    async_tasks.reserve(std::min(tag_info.blob_ids_.size(), kMaxConcurrentDelBlobTasks));

    size_t processed_blobs = 0;
    auto blob_iter = tag_info.blob_ids_.begin();
    
    while (blob_iter != tag_info.blob_ids_.end()) {
      // Create a batch of async tasks (up to kMaxConcurrentDelBlobTasks)
      async_tasks.clear();
      
      for (size_t batch_count = 0; 
           batch_count < kMaxConcurrentDelBlobTasks && blob_iter != tag_info.blob_ids_.end(); 
           ++blob_iter) {
        const auto &[blob_id, _] = *blob_iter;
        
        // Find blob info to get blob name
        auto blob_info_it = blob_id_to_info_.find(blob_id);
        if (blob_info_it != blob_id_to_info_.end()) {
          // Call AsyncDelBlob from client
          auto async_task =
              cte_client->AsyncDelBlob(hipc::MemContext(), tag_id,
                                       blob_info_it->second.blob_name_, blob_id);
          async_tasks.push_back(async_task);
          ++batch_count;
        }
      }

      // Wait for all async DelBlob operations in this batch to complete
      for (auto task : async_tasks) {
        task->Wait();

        // Check if DelBlob succeeded
        if (task->result_code_ != 0) {
          HILOG(
              kWarning,
              "DelBlob failed for blob_id={},{} during tag deletion, continuing",
              task->blob_id_.major_, task->blob_id_.minor_);
          // Continue with other blobs even if one fails
        }

        // Clean up the task
        CHI_IPC->DelTask(task);
        ++processed_blobs;
      }
    }

    // Step 4: Remove all blob name mappings for this tag (DelBlob should have
    // removed them, but ensure cleanup)
    std::string tag_prefix = std::to_string(tag_id.major_) + "." +
                             std::to_string(tag_id.minor_) + ".";
    auto name_it = tag_blob_name_to_id_.begin();
    while (name_it != tag_blob_name_to_id_.end()) {
      if (name_it->first.compare(0, tag_prefix.length(), tag_prefix) == 0) {
        name_it = tag_blob_name_to_id_.erase(name_it);
      } else {
        ++name_it;
      }
    }

    // Step 5: Remove tag name mapping if it exists
    if (!tag_info.tag_name_.empty()) {
      tag_name_to_id_.erase(tag_info.tag_name_);
    }

    // Step 6: Log telemetry and remove tag from tag_id_to_info_ map
    size_t blob_count = processed_blobs;
    size_t total_size = tag_info.total_size_;

    // Log telemetry for DelTag operation
    auto now = std::chrono::steady_clock::now();
    LogTelemetry(CteOp::kDelTag, 0, total_size, BlobId::GetNull(), tag_id, now,
                 now);

    tag_id_to_info_.erase(tag_info_it);

    // Success
    task->result_code_ = 0;
    HILOG(kInfo,
          "DelTag successful: tag_id={},{}, removed {} blobs, total_size={}",
          tag_id.major_, tag_id.minor_, blob_count, total_size);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorDelTag(chi::MonitorModeId mode,
                            hipc::FullPtr<DelTagTask> task,
                            chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate for tag deletion (depends on number of blobs)
    ctx.estimated_completion_time_us = 100.0; // 0.1ms base cost
    break;
  }
}

void Runtime::GetTagSize(hipc::FullPtr<GetTagSizeTask> task,
                         chi::RunContext &ctx) {
  try {
    TagId tag_id = task->tag_id_;

    // Find the tag
    auto tag_info_it = tag_id_to_info_.find(tag_id);
    if (tag_info_it == tag_id_to_info_.end()) {
      task->result_code_ = 1; // Tag not found
      task->tag_size_ = 0;
      return;
    }

    // Update timestamp and return the total size
    auto now = std::chrono::steady_clock::now();
    TagInfo &tag_info = const_cast<TagInfo &>(tag_info_it->second);
    tag_info.last_read_ = now;

    task->tag_size_ = tag_info.total_size_;
    task->result_code_ = 0;

    // Log telemetry for GetTagSize operation
    LogTelemetry(CteOp::kGetTagSize, 0, tag_info.total_size_, BlobId::GetNull(),
                 tag_id, tag_info.last_modified_, now);

    HILOG(kInfo, "GetTagSize successful: tag_id={},{}, total_size={}",
          tag_id.major_, tag_id.minor_, task->tag_size_);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->tag_size_ = 0;
  }
}

void Runtime::MonitorGetTagSize(chi::MonitorModeId mode,
                                hipc::FullPtr<GetTagSizeTask> task,
                                chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate for tag size lookup
    ctx.estimated_completion_time_us = 10.0; // 0.01ms for lookup
    break;
  }
}

// Private helper methods
const Config &Runtime::GetConfig() const {
  auto *config_manager = &ConfigManager::GetInstance();
  return config_manager->GetConfig();
}

void Runtime::UpdateTargetStats(const chi::PoolId &target_id,
                                TargetInfo &target_info) {
  // Get actual statistics from bdev using the GetStats method
  chi::u64 remaining_size;
  chimaera::bdev::PerfMetrics perf_metrics =
      target_info.bdev_client_.GetStats(hipc::MemContext(), remaining_size);

  // Update target info with real performance metrics from bdev
  target_info.perf_metrics_ = perf_metrics;
  target_info.remaining_space_ = remaining_size;

  // Auto-calculate target score using normalized log bandwidth
  double max_bandwidth =
      std::max(target_info.perf_metrics_.read_bandwidth_mbps_,
               target_info.perf_metrics_.write_bandwidth_mbps_);
  if (max_bandwidth > 0.0) {
    // Find the maximum bandwidth across all targets for normalization
    double global_max_bandwidth =
        1000.0; // TODO: Calculate actual max from all targets

    // Use logarithmic scaling for target score: log(bandwidth_i) /
    // log(bandwidth_MAX)
    target_info.target_score_ = static_cast<float>(
        std::log(max_bandwidth + 1.0) / std::log(global_max_bandwidth + 1.0));

    // Clamp to [0, 1] range
    target_info.target_score_ =
        std::max(0.0f, std::min(1.0f, target_info.target_score_));
  } else {
    target_info.target_score_ = 0.0f; // No bandwidth, lowest score
  }
}

TagId Runtime::GetOrAssignTagId(const std::string &tag_name,
                                const TagId &preferred_id) {
  size_t tag_lock_index = GetTagLockIndex(tag_name);
  chi::ScopedCoRwWriteLock write_lock(*tag_locks_[tag_lock_index]);

  // Check if tag already exists
  auto name_it = tag_name_to_id_.find(tag_name);
  if (name_it != tag_name_to_id_.end()) {
    return name_it->second;
  }

  // Assign new tag ID
  TagId tag_id;
  if ((preferred_id.major_ != 0 || preferred_id.minor_ != 0) &&
      tag_id_to_info_.find(preferred_id) == tag_id_to_info_.end()) {
    tag_id = preferred_id;
  } else {
    tag_id = GenerateNewTagId();
  }

  // Create tag info
  TagInfo tag_info(main_allocator_);
  tag_info.tag_name_ = tag_name;
  tag_info.tag_id_ = tag_id;

  // Store mappings
  tag_name_to_id_[tag_name] = tag_id;
  tag_id_to_info_[tag_id] = tag_info;

  return tag_id;
}

// GetWorkRemaining implementation (required pure virtual method)
chi::u64 Runtime::GetWorkRemaining() const {
  // Return approximate work remaining (simple implementation)
  // In a real implementation, this would sum tasks across all queues
  return 0; // For now, always return 0 work remaining
}

// Helper methods for lock index calculation
size_t Runtime::GetTargetLockIndex(const chi::PoolId &target_id) const {
  // Use hash of target_id to distribute locks evenly
  std::hash<chi::PoolId> hasher;
  return hasher(target_id) % target_locks_.size();
}

size_t Runtime::GetTagLockIndex(const std::string &tag_name) const {
  // Use hash of tag name to distribute locks evenly
  std::hash<std::string> hasher;
  return hasher(tag_name) % tag_locks_.size();
}

TagId Runtime::GenerateNewTagId() {
  // Get node_id from IPC manager as the major component
  auto *ipc_manager = CHI_IPC;
  chi::u32 node_id = ipc_manager->GetNodeId();

  // Get next minor component from atomic counter
  chi::u32 minor_id = next_tag_id_minor_.fetch_add(1);

  return TagId{node_id, minor_id};
}

BlobId Runtime::GenerateNewBlobId() {
  // Get node_id from IPC manager as the major component
  auto *ipc_manager = CHI_IPC;
  chi::u32 node_id = ipc_manager->GetNodeId();

  // Get next minor component from atomic counter
  chi::u32 minor_id = next_blob_id_minor_.fetch_add(1);

  return BlobId{node_id, minor_id};
}

size_t Runtime::GetBlobLockIndex(const BlobId &blob_id) const {
  // Use hash of blob ID to distribute locks evenly
  // Hash both major and minor components for good distribution
  std::hash<chi::u32> hasher;
  size_t hash_value = hasher(blob_id.major_) ^ (hasher(blob_id.minor_) << 1);
  return hash_value % tag_locks_.size();
}

// Explicit template instantiations for required template methods
template void Runtime::GetOrCreateTag<CreateParams>(
    hipc::FullPtr<GetOrCreateTagTask<CreateParams>> task, chi::RunContext &ctx);

template void Runtime::MonitorGetOrCreateTag<CreateParams>(
    chi::MonitorModeId mode,
    hipc::FullPtr<GetOrCreateTagTask<CreateParams>> task, chi::RunContext &ctx);

// Blob management helper functions
BlobInfo *Runtime::CheckBlobExists(const BlobId &blob_id,
                                   const std::string &blob_name,
                                   const TagId &tag_id, BlobId &found_blob_id) {
  // Check if blob id is provided and not null
  bool blob_id_provided = (blob_id.major_ != 0 || blob_id.minor_ != 0);
  bool blob_name_provided = !blob_name.empty();

  if (blob_name_provided) {
    // Search by name first using compound key
    std::string compound_key = std::to_string(tag_id.major_) + "." +
                               std::to_string(tag_id.minor_) + "." + blob_name;
    auto blob_name_it = tag_blob_name_to_id_.find(compound_key);
    if (blob_name_it != tag_blob_name_to_id_.end()) {
      const BlobId &blob_id_found = blob_name_it->second;
      found_blob_id = blob_id_found;
      auto blob_it = blob_id_to_info_.find(found_blob_id);
      if (blob_it != blob_id_to_info_.end()) {
        return &blob_id_to_info_[found_blob_id];
      }
    }
  } else if (blob_id_provided) {
    // Search by blob id if no blob name provided
    auto blob_it = blob_id_to_info_.find(blob_id);
    if (blob_it != blob_id_to_info_.end()) {
      found_blob_id = blob_id;
      return &blob_id_to_info_[blob_id];
    }
  }

  // Blob not found
  return nullptr;
}

BlobInfo *Runtime::CreateNewBlob(const std::string &blob_name,
                                 const TagId &tag_id, float blob_score,
                                 BlobId &created_blob_id) {
  // Validate that blob name is provided
  if (blob_name.empty()) {
    return nullptr;
  }

  // Generate new blob ID
  created_blob_id = GenerateNewBlobId();

  // Create new blob info with empty block vector
  BlobInfo new_blob_info(main_allocator_);
  new_blob_info.blob_id_ = created_blob_id;
  new_blob_info.blob_name_ = blob_name;
  new_blob_info.score_ = blob_score;

  // Store blob info in global blob tracking
  blob_id_to_info_[created_blob_id] = new_blob_info;
  BlobInfo *blob_info_ptr = &blob_id_to_info_[created_blob_id];

  // Update tag mappings
  auto tag_it = tag_id_to_info_.find(tag_id);
  if (tag_it != tag_id_to_info_.end()) {
    TagInfo &tag_info = tag_it->second;
    size_t tag_lock_index = GetTagLockIndex(tag_info.tag_name_);
    chi::ScopedCoRwWriteLock tag_lock(*tag_locks_[tag_lock_index]);

    // Add to tag's blob set
    tag_info.blob_ids_[created_blob_id] = 1;

    // Add name-to-id mapping using compound key
    std::string compound_key = std::to_string(tag_id.major_) + "." +
                               std::to_string(tag_id.minor_) + "." + blob_name;
    tag_blob_name_to_id_[compound_key] = created_blob_id;
  }

  return blob_info_ptr;
}

chi::u32 Runtime::AllocateNewData(BlobInfo &blob_info, chi::u64 offset,
                                  chi::u64 size, float blob_score) {
  // Calculate required additional space
  chi::u64 current_blob_size = blob_info.GetTotalSize();
  chi::u64 required_size = offset + size;

  if (required_size <= current_blob_size) {
    // No additional allocation needed
    return 0;
  }

  chi::u64 additional_size = required_size - current_blob_size;

  // Get all available targets for data placement
  size_t lock_index = GetBlobLockIndex(blob_info.blob_id_);
  std::vector<TargetInfo> available_targets;
  {
    chi::ScopedCoRwReadLock read_lock(*target_locks_[lock_index]);

    available_targets.reserve(registered_targets_.size());
    for (const auto &pair : registered_targets_) {
      const TargetInfo &target_info = pair.second;
      available_targets.push_back(target_info);
    }
  }

  if (available_targets.empty()) {
    return 1;
  }

  // Create Data Placement Engine based on configuration
  const Config &config = GetConfig();
  std::unique_ptr<DataPlacementEngine> dpe =
      DpeFactory::CreateDpe(config.dpe_.dpe_type_);

  // Select targets using DPE algorithm before allocation loop
  std::vector<TargetInfo> ordered_targets =
      dpe->SelectTargets(available_targets, blob_score, additional_size);

  if (ordered_targets.empty()) {
    return 1;
  }

  // Use for loop to iterate over pre-selected targets in order
  chi::u64 remaining_to_allocate = additional_size;
  for (const auto &selected_target_info : ordered_targets) {
    // Termination condition: exit when no more space to allocate
    if (remaining_to_allocate == 0) {
      break;
    }

    chi::PoolId selected_target_id = selected_target_info.bdev_client_.pool_id_;

    // Find the selected target info for allocation using TargetId
    size_t target_lock_index = GetTargetLockIndex(selected_target_id);
    TargetInfo *target_info = nullptr;
    {
      chi::ScopedCoRwWriteLock write_lock(*target_locks_[target_lock_index]);
      auto target_it = registered_targets_.find(selected_target_id);
      if (target_it == registered_targets_.end()) {
        continue; // Try next target
      }
      target_info = &target_it->second;

      // Calculate how much we can allocate from this target
      chi::u64 allocate_size =
          std::min(remaining_to_allocate, target_info->remaining_space_);

      if (allocate_size == 0) {
        // No space available, try next target
        continue;
      }

      // Allocate space using bdev client
      chi::u64 allocated_offset;
      if (!AllocateFromTarget(*target_info, allocate_size, allocated_offset)) {
        // Allocation failed, try next target
        continue;
      }

      // Create new block for the allocated space
      BlobBlock new_block(target_info->bdev_client_, allocated_offset,
                          allocate_size);
      blob_info.blocks_.emplace_back(new_block);

      remaining_to_allocate -= allocate_size;
    }
  }

  // Error condition: if we've exhausted all targets but still have remaining
  // space
  if (remaining_to_allocate > 0) {
    return 1;
  }

  return 0; // Success
}

chi::u32 Runtime::ModifyExistingData(const std::vector<BlobBlock> &blocks,
                                     hipc::Pointer data, size_t data_size,
                                     size_t data_offset_in_blob) {
  // Step 1: Initially store the remaining_size equal to data_size
  size_t remaining_size = data_size;

  // Vector to store async write tasks for later waiting
  std::vector<hipc::FullPtr<chimaera::bdev::WriteTask>> write_tasks;
  std::vector<size_t> expected_write_sizes;

  // Step 2: Store the offset of the block in the blob. The first block is
  // offset 0
  size_t block_offset_in_blob = 0;

  // Iterate over every block in the blob
  for (size_t block_idx = 0; block_idx < blocks.size(); ++block_idx) {
    const BlobBlock &block = blocks[block_idx];
    // Step 7: If remaining size is 0, quit the for loop
    if (remaining_size == 0) {
      break;
    }

    // Step 3: Check if the data we are writing is within the range
    // [block_offset_in_blob, block_offset_in_blob + block.size)
    size_t block_end_in_blob = block_offset_in_blob + block.size_;
    size_t data_end_in_blob = data_offset_in_blob + data_size;

    if (data_offset_in_blob < block_end_in_blob &&
        data_end_in_blob > block_offset_in_blob) {
      // Step 4: Clamp the range [data_offset_in_blob, data_offset_in_blob +
      // data_size) to the range [block_offset_in_blob, block_offset_in_blob +
      // block.size)
      size_t write_start_in_blob =
          std::max(data_offset_in_blob, block_offset_in_blob);
      size_t write_end_in_blob = std::min(data_end_in_blob, block_end_in_blob);
      size_t write_size = write_end_in_blob - write_start_in_blob;

      // Calculate offset within the block
      size_t write_start_in_block = write_start_in_blob - block_offset_in_blob;

      // Calculate offset into the data buffer
      size_t data_buffer_offset = write_start_in_blob - data_offset_in_blob;

      // Step 5: Perform async write on the updated range
      chimaera::bdev::Block bdev_block(
          block.target_offset_ + write_start_in_block, write_size, 0);
      hipc::Pointer data_ptr = data + data_buffer_offset;

      chimaera::bdev::Client client_copy = block.bdev_client_;
      auto write_task = client_copy.AsyncWrite(hipc::MemContext(), bdev_block,
                                               data_ptr, write_size);

      write_tasks.push_back(write_task);
      expected_write_sizes.push_back(write_size);

      // Step 6: Subtract the amount of data we have written from the
      // remaining_size
      remaining_size -= write_size;
    }

    // Update block offset for next iteration
    block_offset_in_blob += block.size_;
  }

  // Step 7: Wait for all Async write operations to complete
  for (size_t task_idx = 0; task_idx < write_tasks.size(); ++task_idx) {
    auto task = write_tasks[task_idx];
    size_t expected_size = expected_write_sizes[task_idx];

    task->Wait();

    if (task->bytes_written_ != expected_size) {
      CHI_IPC->DelTask(task);
      return 1;
    }

    CHI_IPC->DelTask(task);
  }

  return 0; // Success
}

chi::u32 Runtime::ReadData(const std::vector<BlobBlock> &blocks,
                           hipc::Pointer data, size_t data_size,
                           size_t data_offset_in_blob) {
  // Step 1: Initially store the remaining_size equal to data_size
  size_t remaining_size = data_size;

  // Vector to store async read tasks for later waiting
  std::vector<hipc::FullPtr<chimaera::bdev::ReadTask>> read_tasks;
  std::vector<size_t> expected_read_sizes;

  // Step 2: Store the offset of the block in the blob. The first block is
  // offset 0
  size_t block_offset_in_blob = 0;

  // Iterate over every block in the blob
  for (size_t block_idx = 0; block_idx < blocks.size(); ++block_idx) {
    const BlobBlock &block = blocks[block_idx];
    // Step 7: If remaining size is 0, quit the for loop
    if (remaining_size == 0) {
      break;
    }

    // Step 3: Check if the data we are reading is within the range
    // [block_offset_in_blob, block_offset_in_blob + block.size)
    size_t block_end_in_blob = block_offset_in_blob + block.size_;
    size_t data_end_in_blob = data_offset_in_blob + data_size;

    if (data_offset_in_blob < block_end_in_blob &&
        data_end_in_blob > block_offset_in_blob) {
      // Step 4: Clamp the range [data_offset_in_blob, data_offset_in_blob +
      // data_size) to the range [block_offset_in_blob, block_offset_in_blob +
      // block.size)
      size_t read_start_in_blob =
          std::max(data_offset_in_blob, block_offset_in_blob);
      size_t read_end_in_blob = std::min(data_end_in_blob, block_end_in_blob);
      size_t read_size = read_end_in_blob - read_start_in_blob;

      // Calculate offset within the block
      size_t read_start_in_block = read_start_in_blob - block_offset_in_blob;

      // Calculate offset into the data buffer
      size_t data_buffer_offset = read_start_in_blob - data_offset_in_blob;

      // Step 5: Perform async read on the range
      chimaera::bdev::Block bdev_block(
          block.target_offset_ + read_start_in_block, read_size, 0);
      hipc::Pointer data_ptr = data + data_buffer_offset;

      chimaera::bdev::Client client_copy = block.bdev_client_;
      auto read_task = client_copy.AsyncRead(hipc::MemContext(), bdev_block,
                                             data_ptr, read_size);

      read_tasks.push_back(read_task);
      expected_read_sizes.push_back(read_size);

      // Step 6: Subtract the amount of data we have read from the
      // remaining_size
      remaining_size -= read_size;
    }

    // Update block offset for next iteration
    block_offset_in_blob += block.size_;
  }

  // Step 7: Wait for all Async read operations to complete
  for (size_t task_idx = 0; task_idx < read_tasks.size(); ++task_idx) {
    auto task = read_tasks[task_idx];
    size_t expected_size = expected_read_sizes[task_idx];

    task->Wait();

    if (task->bytes_read_ != expected_size) {
      CHI_IPC->DelTask(task);
      return 1;
    }

    CHI_IPC->DelTask(task);
  }

  return 0; // Success
}

// Block management helper functions

bool Runtime::AllocateFromTarget(TargetInfo &target_info, chi::u64 size,
                                 chi::u64 &allocated_offset) {
  // Check if target has sufficient space
  if (target_info.remaining_space_ < size) {
    return false;
  }

  try {
    // Use bdev client AllocateBlocks method to get actual offset
    std::vector<chimaera::bdev::Block> allocated_blocks =
        target_info.bdev_client_.AllocateBlocks(hipc::MemContext(), size);

    // Check if we got any blocks
    if (allocated_blocks.empty()) {
      return false;
    }

    // Use the first block (for single allocation case)
    chimaera::bdev::Block allocated_block = allocated_blocks[0];
    allocated_offset = allocated_block.offset_;

    // Update remaining space
    target_info.remaining_space_ -= size;

    return true;
  } catch (const std::exception &e) {
    // Allocation failed
    return false;
  }
}

chi::u32 Runtime::FreeAllBlobBlocks(BlobInfo &blob_info) {
  // Map: PoolId -> vector<Block>
  std::unordered_map<chi::PoolId, std::vector<chimaera::bdev::Block>>
      blocks_by_pool;

  // Group blocks by PoolId
  for (const auto &blob_block : blob_info.blocks_) {
    chi::PoolId pool_id = blob_block.bdev_client_.pool_id_;
    chimaera::bdev::Block block;
    block.offset_ = blob_block.target_offset_;
    block.size_ = blob_block.size_;
    block.block_type_ = 0; // Default block type
    blocks_by_pool[pool_id].push_back(block);
  }

  // Call FreeBlocks once per PoolId
  for (const auto &[pool_id, blocks] : blocks_by_pool) {
    // Get bdev client for this pool from first blob block
    chimaera::bdev::Client bdev_client(pool_id);
    chi::u32 free_result = bdev_client.FreeBlocks(hipc::MemContext(), blocks);
    if (free_result != 0) {
      HILOG(kWarning, "Failed to free blocks from pool {}", pool_id.major_);
    }
  }

  // Clear all blocks
  blob_info.blocks_.clear();
  return 0;
}

void Runtime::LogTelemetry(CteOp op, size_t off, size_t size,
                           const BlobId &blob_id, const TagId &tag_id,
                           const Timestamp &mod_time,
                           const Timestamp &read_time) {
  // Increment atomic counter and get current logical time
  std::uint64_t logical_time = telemetry_counter_.fetch_add(1) + 1;
  
  // Create telemetry entry with logical time and enqueue it
  CteTelemetry telemetry_entry(op, off, size, blob_id, tag_id, mod_time,
                               read_time, logical_time);

  // Circular queue automatically overwrites oldest entries when full
  telemetry_log_.push(telemetry_entry);
}

size_t Runtime::GetTelemetryQueueSize() { return telemetry_log_.GetSize(); }

size_t Runtime::GetTelemetryEntries(std::vector<CteTelemetry> &entries,
                                    size_t max_entries) {
  entries.clear();
  size_t queue_size = telemetry_log_.GetSize();
  size_t entries_to_read = std::min(max_entries, queue_size);

  entries.reserve(entries_to_read);

  // Read entries by popping and re-pushing them (since peek may not be
  // available)
  std::vector<CteTelemetry> temp_entries;
  temp_entries.reserve(entries_to_read);

  // Pop entries temporarily
  for (size_t i = 0; i < entries_to_read; ++i) {
    CteTelemetry entry;
    auto token = telemetry_log_.pop(entry);
    if (!token.IsNull()) {
      temp_entries.push_back(entry);
    } else {
      break; // Queue is empty
    }
  }

  // Re-push entries back to queue (in reverse order to maintain order)
  for (auto it = temp_entries.rbegin(); it != temp_entries.rend(); ++it) {
    telemetry_log_.push(*it);
  }

  // Copy to output vector
  entries = temp_entries;
  return entries.size();
}

void Runtime::PollTelemetryLog(hipc::FullPtr<PollTelemetryLogTask> task,
                               chi::RunContext& ctx) {
  try {
    std::uint64_t minimum_logical_time = task->minimum_logical_time_;
    
    // Get telemetry entries with logical time filtering
    std::vector<CteTelemetry> all_entries;
    size_t retrieved_count = GetTelemetryEntries(all_entries, 1000);
    
    // Filter entries by minimum logical time
    task->entries_.clear();
    std::uint64_t max_logical_time = minimum_logical_time;
    
    for (const auto& entry : all_entries) {
      if (entry.logical_time_ >= minimum_logical_time) {
        task->entries_.emplace_back(entry);
        max_logical_time = std::max(max_logical_time, entry.logical_time_);
      }
    }
    
    task->last_logical_time_ = max_logical_time;
    task->result_code_ = 0;
    
  } catch (const std::exception& e) {
    task->result_code_ = 1;
    task->last_logical_time_ = 0;
  }
  (void)ctx;
}

void Runtime::MonitorPollTelemetryLog(chi::MonitorModeId mode,
                                      hipc::FullPtr<PollTelemetryLogTask> task,
                                      chi::RunContext& ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    auto lane_ptr = GetLaneFullPtr(kStatsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    ctx.estimated_completion_time_us = 100.0;
    break;
  }
  (void)task;
}

void Runtime::GetBlobScore(hipc::FullPtr<GetBlobScoreTask> task,
                          chi::RunContext &ctx) {
  try {
    // Extract input parameters
    TagId tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    BlobId blob_id = task->blob_id_;

    // Validate that either blob_id or blob_name is provided
    bool blob_id_provided = (blob_id.major_ != 0 || blob_id.minor_ != 0);
    bool blob_name_provided = !blob_name.empty();

    if (!blob_id_provided && !blob_name_provided) {
      task->result_code_ = 1;
      return;
    }

    // Step 1: Check if blob exists
    BlobId found_blob_id;
    BlobInfo *blob_info_ptr =
        CheckBlobExists(blob_id, blob_name, tag_id, found_blob_id);

    if (blob_info_ptr == nullptr) {
      task->result_code_ = 1; // Blob not found
      return;
    }

    // Step 2: Return the blob score
    task->score_ = blob_info_ptr->score_;

    // Step 3: Update timestamps and log telemetry
    auto now = std::chrono::steady_clock::now();
    blob_info_ptr->last_read_ = now;

    // No specific telemetry enum for GetBlobScore, using GetBlob as closest match
    LogTelemetry(CteOp::kGetBlob, 0, 0, found_blob_id, tag_id,
                 blob_info_ptr->last_modified_, now);

    // Success
    task->result_code_ = 0;
    HILOG(kInfo, "GetBlobScore successful: blob_id={},{}, name={}, score={}",
          found_blob_id.major_, found_blob_id.minor_, blob_name, 
          blob_info_ptr->score_);

  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorGetBlobScore(chi::MonitorModeId mode,
                                  hipc::FullPtr<GetBlobScoreTask> task,
                                  chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate for blob score lookup
    ctx.estimated_completion_time_us = 10.0; // 0.01ms for lookup
    break;
  }
}

void Runtime::GetBlobSize(hipc::FullPtr<GetBlobSizeTask> task,
                         chi::RunContext &ctx) {
  try {
    // Extract input parameters
    TagId tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    BlobId blob_id = task->blob_id_;
    
    // Validate that either blob_id or blob_name is provided
    bool blob_id_provided = (blob_id.major_ != 0 || blob_id.minor_ != 0);
    bool blob_name_provided = !blob_name.empty();
    if (!blob_id_provided && !blob_name_provided) {
      task->result_code_ = 1;
      return;
    }
    
    // Step 1: Check if blob exists
    BlobId found_blob_id;
    BlobInfo *blob_info_ptr =
        CheckBlobExists(blob_id, blob_name, tag_id, found_blob_id);
    if (blob_info_ptr == nullptr) {
      task->result_code_ = 1; // Blob not found
      return;
    }
    
    // Step 2: Calculate and return the blob size
    task->size_ = blob_info_ptr->GetTotalSize();
    
    // Step 3: Update timestamps and log telemetry
    auto now = std::chrono::steady_clock::now();
    blob_info_ptr->last_read_ = now;
    
    // No specific telemetry enum for GetBlobSize, using GetBlob as closest match
    LogTelemetry(CteOp::kGetBlob, 0, 0, found_blob_id, tag_id,
                 blob_info_ptr->last_modified_, now);
    
    // Success
    task->result_code_ = 0;
    HILOG(kInfo, "GetBlobSize successful: blob_id={},{}, name={}, size={}",
          found_blob_id.major_, found_blob_id.minor_, blob_name, 
          task->size_);
    
  } catch (const std::exception &e) {
    task->result_code_ = 1;
  }
}

void Runtime::MonitorGetBlobSize(chi::MonitorModeId mode,
                                hipc::FullPtr<GetBlobSizeTask> task,
                                chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane *>(lane_ptr.ptr_);
    }
    break;
  }
  case chi::MonitorModeId::kGlobalSchedule:
    break;
  case chi::MonitorModeId::kEstLoad:
    // Estimate for blob size lookup (similar to score lookup)
    ctx.estimated_completion_time_us = 10.0; // 0.01ms for lookup
    break;
  }
}

} // namespace wrp_cte::core

// Define ChiMod entry points using CHI_TASK_CC macro
CHI_TASK_CC(wrp_cte::core::Runtime)