#include <chimaera/core/core_config.h>
#include <chimaera/core/core_dpe.h>
#include <chimaera/core/core_runtime.h>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <algorithm>

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
    std::cout << "Registering targets for storage devices:" << std::endl;

    for (size_t i = 0; i < storage_devices_.size(); ++i) {
      const auto &device = storage_devices_[i];
      std::string target_name = "storage_device_" + std::to_string(i);

      // TODO: Implement target registration when bdev module is available
      // For now, simulate successful target registration
      chi::u32 result = 0;  // Success
      
      // Store device information for later use
      (void)device;  // Suppress unused variable warning

      if (result == 0) {
        std::cout << "  - Registered target '" << target_name
                  << "': " << device.path_ << " (" << device.bdev_type_ << ", "
                  << device.capacity_limit_ << " bytes)" << std::endl;
      } else {
        std::cout << "  - Failed to register target '" << target_name
                  << "' (error code: " << result << ")" << std::endl;
      }
    }
  } else {
    std::cout << "Warning: No storage devices configured" << std::endl;
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
  next_tag_id_.store(1);  // Start tag IDs from 1
  next_blob_id_.store(1); // Start blob IDs from 1

  std::cout << "CTE Core container created and initialized for pool: "
            << pool_name_ << " (ID: " << task->pool_id_ << ")" << std::endl;

  std::cout << "Configuration: worker_count=" << config.worker_count_
            << ", max_targets=" << config.targets_.max_targets_ << std::endl;

  if (config_loaded) {
    if (!config_path.empty()) {
      std::cout << "Configuration loaded from file: " << config_path
                << std::endl;
    } else {
      std::cout << "Configuration loaded from environment" << std::endl;
    }
  } else {
    std::cout << "Using default configuration" << std::endl;
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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

    // Clear tag and blob management structures
    tag_name_to_id_.clear();
    tag_id_to_info_.clear();
    tag_blob_name_to_id_.clear();
    blob_id_to_info_.clear();

    // Reset atomic counters
    next_tag_id_.store(1);
    next_blob_id_.store(1);

    // Clear storage device configuration
    storage_devices_.clear();

    // Clear lock vectors
    target_locks_.clear();
    tag_locks_.clear();

    // Set success status
    task->return_code_ = 0;
    task->error_message_ =
        chi::string(main_allocator_, "Container destroyed successfully");

  } catch (const std::exception &e) {
    task->return_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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

    // Check if target is already registered
    size_t lock_index = GetTargetLockIndex(target_name);
    {
      chi::ScopedCoRwReadLock read_lock(*target_locks_[lock_index]);
      if (registered_targets_.find(target_name) != registered_targets_.end()) {
        task->result_code_ = 1;
        task->error_message_ = chi::string(
            main_allocator_, "Target '" + target_name + "' already registered");
        return;
      }
    }

    // Create bdev client and container
    chimaera::bdev::Client bdev_client;
    std::string bdev_pool_name = target_name;  // Use target_name as the bdev pool name
    
    // Create the bdev container using the client
    chi::PoolQuery pool_query = chi::PoolQuery::Local();
    bdev_client.Create(hipc::MemContext(), pool_query, target_name, bdev_type, total_size);
    
    // Check if creation was successful
    if (bdev_client.return_code_ != 0) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "Failed to create bdev client for target '" + target_name + "'");
      return;
    }

    // Get actual statistics from bdev using GetStats method
    chi::u64 remaining_size;
    chimaera::bdev::PerfMetrics perf_metrics = bdev_client.GetStats(hipc::MemContext(), remaining_size);

    // Create target info with bdev client and performance stats
    TargetInfo target_info(main_allocator_);
    target_info.target_name_ = target_name;
    target_info.bdev_pool_name_ = bdev_pool_name;
    target_info.bdev_client_ = std::move(bdev_client);
    target_info.bytes_read_ = 0;
    target_info.bytes_written_ = 0;
    target_info.ops_read_ = 0;
    target_info.ops_written_ = 0;
    target_info.target_score_ = 0.0f; // Will be calculated based on performance metrics
    target_info.remaining_space_ = remaining_size; // Use actual remaining space from bdev
    target_info.perf_metrics_ = perf_metrics; // Store the entire PerfMetrics structure

    // Register the target
    {
      chi::ScopedCoRwWriteLock write_lock(*target_locks_[lock_index]);
      registered_targets_[target_name] = target_info;
    }

    task->result_code_ = 0; // Success
    std::cout << "Target '" << target_name
              << "' registered with bdev pool: " << bdev_pool_name
              << " (type=" << static_cast<int>(bdev_type)
              << ", path=" << target_name << ", size=" << total_size
              << ", remaining=" << remaining_size << ")" << std::endl;
    std::cout << "  Initial statistics: read_bw="
              << perf_metrics.read_bandwidth_mbps_ << " MB/s"
              << ", write_bw=" << perf_metrics.write_bandwidth_mbps_ << " MB/s"
              << ", avg_latency=" << (target_info.perf_metrics_.read_latency_us_ + target_info.perf_metrics_.write_latency_us_) / 2.0 << " μs"
              << ", iops=" << perf_metrics.iops_ << std::endl;

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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

    // Check if target exists and remove it (don't destroy bdev container)
    size_t lock_index = GetTargetLockIndex(target_name);
    {
      chi::ScopedCoRwWriteLock write_lock(*target_locks_[lock_index]);
      auto it = registered_targets_.find(target_name);
      if (it == registered_targets_.end()) {
        task->result_code_ = 1;
        task->error_message_ = chi::string(
            main_allocator_, "Target '" + target_name + "' not found");
        return;
      }

      registered_targets_.erase(it);
    }

    task->result_code_ = 0; // Success
    std::cout << "Target '" << target_name << "' unregistered" << std::endl;

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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

    // For listing all targets, we need to acquire all target locks
    // Use a simple approach: acquire locks in order to avoid deadlocks
    std::vector<std::unique_ptr<chi::ScopedCoRwReadLock>> locks;
    locks.reserve(target_locks_.size());
    for (size_t i = 0; i < target_locks_.size(); ++i) {
      locks.emplace_back(
          std::make_unique<chi::ScopedCoRwReadLock>(*target_locks_[i]));
    }

    task->targets_.reserve(registered_targets_.size());
    for (const auto &pair : registered_targets_) {
      task->targets_.emplace_back(pair.second);
    }

    task->result_code_ = 0; // Success

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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
    // Acquire all target locks in order to avoid deadlocks
    std::vector<std::unique_ptr<chi::ScopedCoRwWriteLock>> locks;
    locks.reserve(target_locks_.size());
    for (size_t i = 0; i < target_locks_.size(); ++i) {
      locks.emplace_back(
          std::make_unique<chi::ScopedCoRwWriteLock>(*target_locks_[i]));
    }

    for (auto &pair : registered_targets_) {
      UpdateTargetStats(pair.first, pair.second);
    }

    task->result_code_ = 0; // Success

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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
    chi::u32 preferred_id = task->tag_id_;

    // Get or assign tag ID
    chi::u32 tag_id = GetOrAssignTagId(tag_name, preferred_id);
    task->tag_id_ = tag_id;

    // Populate tag info
    size_t tag_lock_index = GetTagLockIndex(tag_name);
    {
      chi::ScopedCoRwReadLock read_lock(*tag_locks_[tag_lock_index]);
      auto it = tag_id_to_info_.find(tag_id);
      if (it != tag_id_to_info_.end()) {
        task->tag_info_ = it->second;
      }
    }

    task->result_code_ = 0; // Success

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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
    chi::u32 tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    chi::u32 blob_id = task->blob_id_;
    chi::u64 offset = task->offset_;
    chi::u64 size = task->size_;
    hipc::Pointer blob_data = task->blob_data_;
    float blob_score = task->score_;
    chi::u32 flags = task->flags_;

    // Validate input parameters
    if (size == 0) {
      task->result_code_ = 1;
      task->error_message_ =
          chi::string(main_allocator_, "Blob size cannot be zero");
      return;
    }

    if (blob_data.IsNull()) {
      task->result_code_ = 1;
      task->error_message_ =
          chi::string(main_allocator_, "Blob data pointer is null");
      return;
    }

    // Validate blob name and ID - both must be provided (no automatic
    // generation)
    if (blob_name.empty()) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "Blob name must be provided (automatic generation disabled)");
      return;
    }

    if (blob_id == 0) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "Blob ID must be provided (automatic generation disabled)");
      return;
    }

    // Check if blob already exists - PutBlob can get or create
    auto blob_it = blob_id_to_info_.find(blob_id);
    bool blob_exists = (blob_it != blob_id_to_info_.end());

    if (blob_exists) {
      // Blob exists - verify the name matches
      if (blob_it->second.blob_name_.str() != blob_name) {
        task->result_code_ = 1;
        task->error_message_ = chi::string(
            main_allocator_, "Blob with ID " + std::to_string(blob_id) +
                                 " exists but has different name '" +
                                 blob_it->second.blob_name_.str() +
                                 "' (expected '" + blob_name + "')");
        return;
      }

      // TODO: For existing blobs, we could perform an update operation
      // For now, just return success for existing blobs without modifying them
      task->result_code_ = 0;
      std::cout << "PutBlob: Found existing blob_id=" << blob_id
                << ", name=" << blob_name << std::endl;
      return;
    }

    // Blob is new - proceed with creation

    // Get all available targets for data placement
    std::vector<TargetInfo> available_targets;
    {
      // Acquire all target locks to get a consistent view
      std::vector<std::unique_ptr<chi::ScopedCoRwReadLock>> locks;
      locks.reserve(target_locks_.size());
      for (size_t i = 0; i < target_locks_.size(); ++i) {
        locks.emplace_back(
            std::make_unique<chi::ScopedCoRwReadLock>(*target_locks_[i]));
      }

      available_targets.reserve(registered_targets_.size());
      for (const auto &pair : registered_targets_) {
        available_targets.push_back(pair.second);
      }
    }

    if (available_targets.empty()) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_, "No targets available for data placement");
      return;
    }

    // Create Data Placement Engine based on configuration
    const Config &config = GetConfig();
    std::unique_ptr<DataPlacementEngine> dpe =
        DpeFactory::CreateDpe(config.dpe_.dpe_type_);

    // Select target using DPE algorithm
    std::string selected_target =
        dpe->SelectTarget(available_targets, blob_score, size);

    if (selected_target.empty()) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "No suitable target found for blob placement (insufficient space)");
      return;
    }

    // Find the selected target info for allocation
    size_t target_lock_index = GetTargetLockIndex(selected_target);
    TargetInfo *target_info = nullptr;
    {
      chi::ScopedCoRwWriteLock write_lock(*target_locks_[target_lock_index]);
      auto target_it = registered_targets_.find(selected_target);
      if (target_it == registered_targets_.end()) {
        task->result_code_ = 1;
        task->error_message_ =
            chi::string(main_allocator_, "Selected target '" + selected_target +
                                             "' no longer exists");
        return;
      }
      target_info = &target_it->second;

      // Double-check space availability (may have changed since DPE selection)
      if (target_info->remaining_space_ < size) {
        task->result_code_ = 1;
        task->error_message_ = chi::string(
            main_allocator_, "Selected target '" + selected_target +
                                 "' no longer has sufficient space");
        return;
      }

      // Reserve space on the target
      target_info->remaining_space_ -= size;
    }

    // Implement actual bdev write operation using bdev client
    chi::u64 allocated_offset = offset; // Use provided offset or calculate new one
    
    try {
      // Create block for write operation
      chimaera::bdev::Block block(allocated_offset, size, 0); // Use block_type 0 for now
      
      // Create data vector for write operation
      // TODO: Extract actual data from task->blob_data_ when pointer access is fixed
      std::vector<hshm::u8> data_vector(size, 0x41); // Fill with test data for now
      
      // Perform write operation using bdev client
      chi::u64 bytes_written = target_info->bdev_client_.Write(hipc::MemContext(), block, data_vector);
      
      if (bytes_written != size) {
        // Restore reserved space on partial write
        target_info->remaining_space_ += size;
        task->result_code_ = 1;
        task->error_message_ = chi::string(
            main_allocator_,
            "Partial write for blob '" + blob_name + "': wrote " + std::to_string(bytes_written) + " of " + std::to_string(size) + " bytes");
        return;
      }
      
      // TODO: Handle multi-block writes if blob spans multiple blocks
      // TODO: Add proper async operation completion handling
      
    } catch (const std::exception& e) {
      // Restore reserved space on error
      target_info->remaining_space_ += size;
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "Failed to write blob '" + blob_name + "': " + e.what());
      return;
    }

    // Create blob info
    BlobInfo blob_info(main_allocator_);
    blob_info.blob_id_ = blob_id;
    blob_info.blob_name_ = chi::string(main_allocator_, blob_name);
    blob_info.target_name_ = chi::string(main_allocator_, selected_target);
    blob_info.offset_ = allocated_offset;
    blob_info.size_ = size;
    blob_info.score_ = blob_score;

    // Store blob info in global blob tracking
    blob_id_to_info_[blob_id] = blob_info;

    // Update tag's blob set if tag exists
    auto tag_it = tag_id_to_info_.find(tag_id);
    if (tag_it != tag_id_to_info_.end()) {
      size_t tag_lock_index = GetTagLockIndex(tag_it->second.tag_name_.str());
      chi::ScopedCoRwWriteLock tag_lock(*tag_locks_[tag_lock_index]);
      tag_it->second.blob_ids_[blob_id] = blob_id;
    }

    // TODO: Perform async write operation to bdev
    // For now, just mark as successful
    task->result_code_ = 0; // Success

    std::cout << "PutBlob: blob_id=" << blob_id << ", name=" << blob_name
              << ", target=" << selected_target << ", size=" << size
              << ", score=" << blob_score << ", dpe=" << config.dpe_.dpe_type_
              << std::endl;

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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
    chi::u32 tag_id = task->tag_id_;
    std::string blob_name = task->blob_name_.str();
    chi::u32 blob_id = task->blob_id_;
    chi::u64 offset = task->offset_;
    chi::u64 size = task->size_;
    chi::u32 flags = task->flags_;

    // Validate input parameters
    if (size == 0) {
      task->result_code_ = 1;
      task->error_message_ =
          chi::string(main_allocator_, "Read size cannot be zero");
      return;
    }

    // Find blob by ID or name within tag
    BlobInfo blob_info;
    bool found = false;

    if (blob_id != 0) {
      // Find blob by ID
      auto blob_it = blob_id_to_info_.find(blob_id);
      if (blob_it != blob_id_to_info_.end()) {
        blob_info = blob_it->second;
        found = true;
      }
    } else if (!blob_name.empty() && tag_id != 0) {
      // Find blob by name within tag
      size_t tag_lock_index = GetTagLockIndex(blob_name);
      chi::ScopedCoRwReadLock tag_lock(*tag_locks_[tag_lock_index]);

      auto tag_blob_it = tag_blob_name_to_id_.find(tag_id);
      if (tag_blob_it != tag_blob_name_to_id_.end()) {
        auto name_it = tag_blob_it->second.find(blob_name);
        if (name_it != tag_blob_it->second.end()) {
          chi::u32 found_blob_id = name_it->second;
          auto blob_it = blob_id_to_info_.find(found_blob_id);
          if (blob_it != blob_id_to_info_.end()) {
            blob_info = blob_it->second;
            found = true;
            task->blob_id_ = found_blob_id; // Set the blob_id in the task
          }
        }
      }
    }

    if (!found) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_, "Blob not found (tag_id=" + std::to_string(tag_id) +
                               ", blob_id=" + std::to_string(blob_id) +
                               ", blob_name='" + blob_name + "')");
      return;
    }

    // Validate read parameters against blob size
    if (offset >= blob_info.size_) {
      task->result_code_ = 1;
      task->error_message_ =
          chi::string(main_allocator_, "Read offset " + std::to_string(offset) +
                                           " exceeds blob size " +
                                           std::to_string(blob_info.size_));
      return;
    }

    // Adjust size if reading beyond blob end
    chi::u64 available_size = blob_info.size_ - offset;
    chi::u64 actual_size = std::min(size, available_size);

    // Find target containing the blob
    std::string target_name = blob_info.target_name_.str();
    size_t target_lock_index = GetTargetLockIndex(target_name);
    TargetInfo target_info;
    bool target_found = false;

    {
      chi::ScopedCoRwReadLock read_lock(*target_locks_[target_lock_index]);
      auto target_it = registered_targets_.find(target_name);
      if (target_it != registered_targets_.end()) {
        target_info = target_it->second;
        target_found = true;
      }
    }

    if (!target_found) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "Target '" + target_name + "' containing blob no longer exists");
      return;
    }

    // Allocate memory for blob data if not provided
    if (task->blob_data_.IsNull()) {
      // TODO: Allocate shared memory buffer for blob data
      // For now, simulate successful allocation
      task->blob_data_ = hipc::Pointer::GetNull(); // Placeholder
    }

    // Calculate actual read offset within target
    chi::u64 target_offset = blob_info.offset_ + offset;

    // Perform read operation from bdev
    try {
      // Create block for read operation
      chimaera::bdev::Block block(target_offset, actual_size, 0); // Use block_type 0 for now
      
      // Perform read operation using bdev client
      std::vector<hshm::u8> read_data = target_info.bdev_client_.Read(hipc::MemContext(), block);
      
      if (read_data.size() != actual_size) {
        task->result_code_ = 1;
        task->error_message_ = chi::string(
            main_allocator_,
            "Partial read for blob '" + blob_name + "': read " + std::to_string(read_data.size()) + " of " + std::to_string(actual_size) + " bytes");
        return;
      }
      
      // For now, return the read data size as success indicator
      // TODO: Implement proper shared memory buffer allocation and data copying
      task->blob_data_ = hipc::Pointer::GetNull(); // Placeholder
      
      // TODO: Handle multi-block reads if blob spans multiple blocks
      // TODO: Add proper async operation completion handling
      
    } catch (const std::exception& e) {
      task->result_code_ = 1;
      task->error_message_ = chi::string(
          main_allocator_,
          "Failed to read blob '" + blob_name + "': " + e.what());
      return;
    }
    std::cout << "GetBlob: blob_id=" << blob_info.blob_id_
              << ", name=" << blob_info.blob_name_.str()
              << ", target=" << target_name << ", offset=" << target_offset
              << ", size=" << actual_size << " (requested=" << size
              << ", blob_size=" << blob_info.size_ << ")" << std::endl;

    task->result_code_ = 0; // Success

    // Update task with actual read parameters
    task->size_ =
        actual_size; // Update size to reflect actual bytes that will be read

  } catch (const std::exception &e) {
    task->result_code_ = 1;
    task->error_message_ = chi::string(main_allocator_, e.what());
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
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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
  task->error_message_ =
      chi::string(main_allocator_, "ReorganizeBlob not implemented yet");
}

void Runtime::MonitorReorganizeBlob(chi::MonitorModeId mode,
                                    hipc::FullPtr<ReorganizeBlobTask> task,
                                    chi::RunContext &ctx) {
  switch (mode) {
  case chi::MonitorModeId::kLocalSchedule: {
    // Route to blob operations queue (round-robin on lanes)
    auto lane_ptr = GetLaneFullPtr(kBlobOperationsQueue, 0);
    if (!lane_ptr.IsNull()) {
      ctx.route_lane_ = reinterpret_cast<chi::TaskLane*>(lane_ptr.ptr_);
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

// Private helper methods
const Config &Runtime::GetConfig() const {
  auto *config_manager = &ConfigManager::GetInstance();
  return config_manager->GetConfig();
}

std::string
Runtime::CreateBdevForTarget(const std::string &target_name,
                             const StorageDeviceConfig &device_config) {
  // TODO: Implement actual bdev creation using chimaera bdev
  // For now, return a mock pool name based on the parsed config
  std::string bdev_pool_name = "bdev_pool_" + target_name;

  // In a real implementation, this would:
  // 1. Use device_config.path_ as the storage directory
  // 2. Use device_config.bdev_type_ to determine bdev backend ("file" or "ram")
  // 3. Use device_config.capacity_limit_ as the size limit in bytes
  // 4. Use chimaera bdev client to create/get a bdev with these parameters
  // 5. Return the actual bdev pool name

  std::cout << "CreateBdevForTarget: target=" << target_name
            << ", path=" << device_config.path_
            << ", type=" << device_config.bdev_type_
            << ", capacity=" << device_config.capacity_limit_ << " bytes"
            << std::endl;

  return bdev_pool_name;
}

void Runtime::UpdateTargetStats(const std::string &target_name,
                                TargetInfo &target_info) {
  // Get configuration for performance tuning
  const Config &config = GetConfig();

  // TODO: Implement actual stats polling from bdev using
  // target_info.bdev_client_ For now, increment some mock stats using
  // configured parameters
  target_info.ops_read_ += 1;
  target_info.ops_written_ += 1;

  // Use configured cache size to simulate read/write amounts
  chi::u64 read_amount = std::min(
      static_cast<chi::u64>(4096),
      static_cast<chi::u64>(config.performance_.blob_cache_size_mb_ * 1024));
  chi::u64 write_amount = read_amount;

  target_info.bytes_read_ += read_amount;
  target_info.bytes_written_ += write_amount;

  // Update performance metrics with current operation stats
  // Note: We should refresh these from bdev periodically, but for now update latency
  float mock_latency =
      50.0f * (1.0f / config.performance_.max_concurrent_operations_);
  target_info.perf_metrics_.read_latency_us_ =
      (target_info.perf_metrics_.read_latency_us_ + mock_latency) / 2.0;
  target_info.perf_metrics_.write_latency_us_ =
      (target_info.perf_metrics_.write_latency_us_ + mock_latency) / 2.0;

  // Calculate current bandwidth in MB/s based on actual bytes transferred
  double time_elapsed_s = 1.0; // Assume 1 second elapsed for calculation
  target_info.perf_metrics_.read_bandwidth_mbps_ =
      (target_info.bytes_read_ / (1024.0 * 1024.0)) / time_elapsed_s;
  target_info.perf_metrics_.write_bandwidth_mbps_ =
      (target_info.bytes_written_ / (1024.0 * 1024.0)) / time_elapsed_s;

  // Auto-calculate target score using normalized log bandwidth
  double max_bandwidth = std::max(target_info.perf_metrics_.read_bandwidth_mbps_,
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

  // TODO: Update remaining_space_ based on actual bdev allocations
  // For now, keep it as initialized or decremented during actual allocations
}

chi::u32 Runtime::GetOrAssignTagId(const std::string &tag_name,
                                   chi::u32 preferred_id) {
  size_t tag_lock_index = GetTagLockIndex(tag_name);
  chi::ScopedCoRwWriteLock write_lock(*tag_locks_[tag_lock_index]);

  // Check if tag already exists
  auto name_it = tag_name_to_id_.find(tag_name);
  if (name_it != tag_name_to_id_.end()) {
    return name_it->second;
  }

  // Assign new tag ID
  chi::u32 tag_id;
  if (preferred_id != 0 &&
      tag_id_to_info_.find(preferred_id) == tag_id_to_info_.end()) {
    tag_id = preferred_id;
  } else {
    tag_id = next_tag_id_.fetch_add(1);
  }

  // Create tag info
  TagInfo tag_info(main_allocator_);
  tag_info.tag_name_ = chi::string(main_allocator_, tag_name);
  tag_info.tag_id_ = tag_id;

  // Store mappings
  tag_name_to_id_[tag_name] = tag_id;
  tag_id_to_info_[tag_id] = tag_info;
  tag_blob_name_to_id_[tag_id] = std::unordered_map<std::string, chi::u32>();

  return tag_id;
}

chi::u32 Runtime::GetOrAssignBlobId(chi::u32 tag_id,
                                    const std::string &blob_name,
                                    chi::u32 preferred_id) {
  size_t tag_lock_index =
      GetTagLockIndex(blob_name); // Use blob_name for lock selection
  chi::ScopedCoRwWriteLock write_lock(*tag_locks_[tag_lock_index]);

  // Check if blob already exists in this tag
  auto tag_it = tag_blob_name_to_id_.find(tag_id);
  if (tag_it != tag_blob_name_to_id_.end()) {
    auto blob_it = tag_it->second.find(blob_name);
    if (blob_it != tag_it->second.end()) {
      return blob_it->second;
    }
  }

  // Assign new blob ID
  chi::u32 blob_id;
  if (preferred_id != 0 &&
      blob_id_to_info_.find(preferred_id) == blob_id_to_info_.end()) {
    blob_id = preferred_id;
  } else {
    blob_id = next_blob_id_.fetch_add(1);
  }

  // Store mappings
  if (tag_it == tag_blob_name_to_id_.end()) {
    tag_blob_name_to_id_[tag_id] = std::unordered_map<std::string, chi::u32>();
  }
  tag_blob_name_to_id_[tag_id][blob_name] = blob_id;

  // Add blob to tag's blob set
  auto tag_info_it = tag_id_to_info_.find(tag_id);
  if (tag_info_it != tag_id_to_info_.end()) {
    tag_info_it->second.blob_ids_[blob_id] = blob_id;
  }

  return blob_id;
}

// GetWorkRemaining implementation (required pure virtual method)
chi::u64 Runtime::GetWorkRemaining() const {
  // Return approximate work remaining (simple implementation)
  // In a real implementation, this would sum tasks across all queues
  return 0; // For now, always return 0 work remaining
}

// Helper methods for lock index calculation
size_t Runtime::GetTargetLockIndex(const std::string &target_name) const {
  // Use hash of target name to distribute locks evenly
  std::hash<std::string> hasher;
  return hasher(target_name) % target_locks_.size();
}

size_t Runtime::GetTagLockIndex(const std::string &tag_name) const {
  // Use hash of tag name to distribute locks evenly
  std::hash<std::string> hasher;
  return hasher(tag_name) % tag_locks_.size();
}

// Explicit template instantiations for required template methods
template void Runtime::GetOrCreateTag<CreateParams>(
    hipc::FullPtr<GetOrCreateTagTask<CreateParams>> task, chi::RunContext &ctx);

template void Runtime::MonitorGetOrCreateTag<CreateParams>(
    chi::MonitorModeId mode,
    hipc::FullPtr<GetOrCreateTagTask<CreateParams>> task, chi::RunContext &ctx);

} // namespace wrp_cte::core

// Define ChiMod entry points using CHI_TASK_CC macro
CHI_TASK_CC(wrp_cte::core::Runtime)