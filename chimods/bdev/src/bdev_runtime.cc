#include <chimaera/bdev/bdev_runtime.h>
#include <chrono>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace chimaera::bdev_extended {

// Static member definitions
chi::CoRwLock Runtime::data_lock_;
chi::CoMutex Runtime::target_mutex_;
chi::CoMutex Runtime::perf_mutex_;

// ==============================================================================
// BlockAllocator Implementation
// ==============================================================================

void BlockAllocator::Initialize(chi::u64 total_size) {
  total_size_ = total_size;
  allocated_size_ = 0;
  next_offset_ = 0;
  
  // Clear free lists
  free_blocks_4kb_.clear();
  free_blocks_64kb_.clear();
  free_blocks_256kb_.clear();
  free_blocks_1mb_.clear();
}

Block BlockAllocator::Allocate(chi::u64 size) {
  chi::u32 block_type = GetBlockType(size);
  chi::u64 block_size = GetBlockSizeForType(block_type);
  
  // Check free list first
  auto& free_list = GetFreeListForType(block_type);
  if (!free_list.empty()) {
    Block block = free_list.back();
    free_list.pop_back();
    return block;
  }
  
  // Allocate new block if space available
  if (next_offset_ + block_size <= total_size_) {
    Block block(next_offset_, block_size, block_type);
    next_offset_ += block_size;
    allocated_size_ += block_size;
    return block;
  }
  
  // No space available
  return Block(0, 0, 0);
}

bool BlockAllocator::Free(const Block& block) {
  if (block.size_ == 0) return false;
  
  // Add to appropriate free list
  auto& free_list = GetFreeListForType(block.block_type_);
  free_list.push_back(block);
  allocated_size_ -= block.size_;
  return true;
}

chi::u32 BlockAllocator::GetBlockType(chi::u64 size) const {
  if (size <= kBlockSize4KB) return 0;
  if (size <= kBlockSize64KB) return 1;
  if (size <= kBlockSize256KB) return 2;
  return 3; // 1MB blocks
}

chi::u64 BlockAllocator::GetBlockSizeForType(chi::u32 block_type) const {
  switch (block_type) {
    case 0: return kBlockSize4KB;
    case 1: return kBlockSize64KB;
    case 2: return kBlockSize256KB;
    case 3: return kBlockSize1MB;
    default: return kBlockSize4KB;
  }
}

std::vector<Block>& BlockAllocator::GetFreeListForType(chi::u32 block_type) {
  switch (block_type) {
    case 0: return free_blocks_4kb_;
    case 1: return free_blocks_64kb_;
    case 2: return free_blocks_256kb_;
    case 3: return free_blocks_1mb_;
    default: return free_blocks_4kb_;
  }
}

// ==============================================================================
// FileBackend Implementation
// ==============================================================================

bool FileBackend::Initialize(const std::string& file_path, chi::u32 io_depth, chi::u32 alignment) {
#ifdef __linux__
  // Open file with direct I/O flags for better performance
  fd_ = open(file_path.c_str(), O_RDWR | O_DIRECT);
  if (fd_ < 0) {
    // Fallback without O_DIRECT if not supported
    fd_ = open(file_path.c_str(), O_RDWR);
    if (fd_ < 0) {
      return false;
    }
  }
  
#ifdef CHIMAERA_BDEV_HAS_AIO
  // Initialize libaio context
  ctx_ = nullptr;
  if (io_setup(io_depth, &ctx_) < 0) {
    close(fd_);
    fd_ = -1;
    return false;
  }
#else
  // Placeholder when libaio is not available
  ctx_ = nullptr;
  (void)io_depth;  // Suppress unused parameter warning
#endif
  
  alignment_ = alignment;
  return true;
#else
  // Fallback for non-Linux systems
  fd_ = open(file_path.c_str(), O_RDWR);
  alignment_ = alignment;
  ctx_ = nullptr;
  (void)io_depth;  // Suppress unused parameter warning
  return fd_ >= 0;
#endif
}

void FileBackend::Cleanup() {
#ifdef CHIMAERA_BDEV_HAS_AIO
  if (ctx_ != nullptr) {
    io_destroy(ctx_);
    ctx_ = nullptr;
  }
#endif
  
  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

chi::u64 FileBackend::Write(const Block& block, const std::vector<hshm::u8>& data) {
  if (fd_ < 0 || data.empty()) return 0;
  
#ifdef CHIMAERA_BDEV_HAS_AIO
  // Use aligned buffer for direct I/O
  void* aligned_buffer = AlignedAlloc(data.size());
  if (!aligned_buffer) return 0;
  
  std::memcpy(aligned_buffer, data.data(), data.size());
  
  ssize_t bytes_written = pwrite(fd_, aligned_buffer, data.size(), block.offset_);
  AlignedFree(aligned_buffer);
  
  return bytes_written > 0 ? static_cast<chi::u64>(bytes_written) : 0;
#else
  // Simple write for non-Linux systems
  ssize_t bytes_written = pwrite(fd_, data.data(), data.size(), block.offset_);
  return bytes_written > 0 ? static_cast<chi::u64>(bytes_written) : 0;
#endif
}

std::vector<hshm::u8> FileBackend::Read(const Block& block) {
  std::vector<hshm::u8> result;
  if (fd_ < 0 || block.size_ == 0) return result;
  
#ifdef CHIMAERA_BDEV_HAS_AIO
  // Use aligned buffer for direct I/O
  void* aligned_buffer = AlignedAlloc(block.size_);
  if (!aligned_buffer) return result;
  
  ssize_t bytes_read = pread(fd_, aligned_buffer, block.size_, block.offset_);
  if (bytes_read > 0) {
    result.resize(bytes_read);
    std::memcpy(result.data(), aligned_buffer, bytes_read);
  }
  
  AlignedFree(aligned_buffer);
#else
  // Simple read for non-Linux systems
  result.resize(block.size_);
  ssize_t bytes_read = pread(fd_, result.data(), block.size_, block.offset_);
  if (bytes_read > 0) {
    result.resize(bytes_read);
  } else {
    result.clear();
  }
#endif
  
  return result;
}

#ifdef CHIMAERA_BDEV_HAS_AIO
void* FileBackend::AlignedAlloc(size_t size) {
  void* ptr = nullptr;
  if (posix_memalign(&ptr, alignment_, (size + alignment_ - 1) & ~(alignment_ - 1)) != 0) {
    return nullptr;
  }
  return ptr;
}

void FileBackend::AlignedFree(void* ptr) {
  if (ptr) {
    free(ptr);
  }
}
#endif

// ==============================================================================
// RamBackend Implementation
// ==============================================================================

bool RamBackend::Initialize(chi::u64 total_size) {
  buffer_ = std::malloc(total_size);
  if (!buffer_) return false;
  
  size_ = total_size;
  std::memset(buffer_, 0, total_size);
  return true;
}

void RamBackend::Cleanup() {
  if (buffer_) {
    std::free(buffer_);
    buffer_ = nullptr;
  }
  size_ = 0;
}

chi::u64 RamBackend::Write(const Block& block, const std::vector<hshm::u8>& data) {
  if (!buffer_ || block.offset_ + data.size() > size_) return 0;
  
  std::memcpy(static_cast<char*>(buffer_) + block.offset_, data.data(), data.size());
  return data.size();
}

std::vector<hshm::u8> RamBackend::Read(const Block& block) {
  std::vector<hshm::u8> result;
  if (!buffer_ || block.offset_ + block.size_ > size_) return result;
  
  result.resize(block.size_);
  std::memcpy(result.data(), static_cast<char*>(buffer_) + block.offset_, block.size_);
  return result;
}

// ==============================================================================
// Runtime Implementation
// ==============================================================================

void Runtime::Create(hipc::FullPtr<CreateTask> task, chi::RunContext& ctx) {
  auto create_params = task->GetParams(main_allocator_);
  
  // Store configuration
  bdev_type_ = create_params.bdev_type_;
  file_path_ = create_params.file_path_;
  total_size_ = create_params.total_size_;
  io_depth_ = create_params.io_depth_;
  alignment_ = create_params.alignment_;
  
  // Initialize appropriate backend
  bool success = false;
  if (bdev_type_ == BdevType::kFile) {
    file_backend_ = std::make_unique<FileBackend>();
    success = file_backend_->Initialize(file_path_, io_depth_, alignment_);
    
    // Get file size if total_size is 0
    if (success && total_size_ == 0) {
      struct stat file_stat;
      if (stat(file_path_.c_str(), &file_stat) == 0) {
        total_size_ = static_cast<chi::u64>(file_stat.st_size);
      }
    }
  } else if (bdev_type_ == BdevType::kRam) {
    if (total_size_ == 0) {
      // Error: RAM backend requires non-zero size
      return;
    }
    ram_backend_ = std::make_unique<RamBackend>();
    success = ram_backend_->Initialize(total_size_);
  }
  
  if (!success) {
      return;
  }
  
  // Initialize block allocator
  block_allocator_.Initialize(total_size_);
  
  // Initialize performance metrics
  perf_metrics_ = PerfMetrics();
  total_reads_ = 0;
  total_writes_ = 0;
  total_read_time_ = 0.0;
  total_write_time_ = 0.0;
  
  // Initialize target registry
  registered_targets_.clear();
  
}

void Runtime::MonitorCreate(chi::MonitorModeId mode, hipc::FullPtr<CreateTask> task,
                           chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kLocalSchedule: {
      // Route to metadata queue for container initialization
      auto lane_ptr = GetLaneFullPtr(kMetadataQueue, 0);
      if (!lane_ptr.IsNull()) {
        ctx.route_lane_ = static_cast<void *>(lane_ptr.ptr_);
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

void Runtime::Allocate(hipc::FullPtr<AllocateTask> task, chi::RunContext& ctx) {
  chi::ScopedCoRwWriteLock write_lock(data_lock_);
  
  Block block = block_allocator_.Allocate(task->size_);
  if (block.size_ == 0) {
    task->result_code_ = 1; // Allocation failed
  } else {
    task->block_ = block;
    task->result_code_ = 0; // Success
  }
  
}

void Runtime::MonitorAllocate(chi::MonitorModeId mode, hipc::FullPtr<AllocateTask> task,
                             chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 500; // 0.5ms estimated load
      break;
    }
  }
}

void Runtime::Free(hipc::FullPtr<FreeTask> task, chi::RunContext& ctx) {
  chi::ScopedCoRwWriteLock write_lock(data_lock_);
  
  bool success = block_allocator_.Free(task->block_);
  task->result_code_ = success ? 0 : 1;
  
}

void Runtime::MonitorFree(chi::MonitorModeId mode, hipc::FullPtr<FreeTask> task,
                         chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 100; // 0.1ms estimated load
      break;
    }
  }
}

void Runtime::Write(hipc::FullPtr<WriteTask> task, chi::RunContext& ctx) {
  auto start_time = std::chrono::high_resolution_clock::now();
  
  chi::u64 bytes_written = 0;
  if (bdev_type_ == BdevType::kFile && file_backend_) {
    std::vector<hshm::u8> data;
    data.resize(task->data_.size());
    for (size_t i = 0; i < task->data_.size(); ++i) {
      data[i] = task->data_[i];
    }
    bytes_written = file_backend_->Write(task->block_, data);
  } else if (bdev_type_ == BdevType::kRam && ram_backend_) {
    std::vector<hshm::u8> data;
    data.resize(task->data_.size());
    for (size_t i = 0; i < task->data_.size(); ++i) {
      data[i] = task->data_[i];
    }
    bytes_written = ram_backend_->Write(task->block_, data);
  }
  
  auto end_time = std::chrono::high_resolution_clock::now();
  double time_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();
  
  task->bytes_written_ = bytes_written;
  task->result_code_ = (bytes_written > 0) ? 0 : 1;
  
  // Update performance metrics
  UpdatePerformanceMetrics(false, bytes_written, time_us);
  
}

void Runtime::MonitorWrite(chi::MonitorModeId mode, hipc::FullPtr<WriteTask> task,
                          chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 10000; // 10ms estimated load
      break;
    }
  }
}

void Runtime::Read(hipc::FullPtr<ReadTask> task, chi::RunContext& ctx) {
  auto start_time = std::chrono::high_resolution_clock::now();
  
  std::vector<hshm::u8> data;
  if (bdev_type_ == BdevType::kFile && file_backend_) {
    data = file_backend_->Read(task->block_);
  } else if (bdev_type_ == BdevType::kRam && ram_backend_) {
    data = ram_backend_->Read(task->block_);
  }
  
  auto end_time = std::chrono::high_resolution_clock::now();
  double time_us = std::chrono::duration<double, std::micro>(end_time - start_time).count();
  
  // Copy data to task's HSHM vector
  task->data_.resize(data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    task->data_[i] = data[i];
  }
  
  task->bytes_read_ = data.size();
  task->result_code_ = data.empty() ? 1 : 0;
  
  // Update performance metrics
  UpdatePerformanceMetrics(true, data.size(), time_us);
  
}

void Runtime::MonitorRead(chi::MonitorModeId mode, hipc::FullPtr<ReadTask> task,
                         chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 5000; // 5ms estimated load
      break;
    }
  }
}

void Runtime::GetStats(hipc::FullPtr<StatTask> task, chi::RunContext& ctx) {
  chi::ScopedCoMutex perf_lock(perf_mutex_);
  chi::ScopedCoRwReadLock read_lock(data_lock_);
  
  task->metrics_ = perf_metrics_;
  task->remaining_size_ = block_allocator_.GetRemainingSize();
  task->result_code_ = 0;
  
}

void Runtime::MonitorGetStats(chi::MonitorModeId mode, hipc::FullPtr<StatTask> task,
                             chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 100; // 0.1ms estimated load
      break;
    }
  }
}

// ==============================================================================
// Target Registration Operations
// ==============================================================================

void Runtime::RegisterTarget(hipc::FullPtr<RegisterTargetTask> task, chi::RunContext& ctx) {
  chi::ScopedCoMutex target_lock(target_mutex_);
  
  std::string target_name = task->target_name_.str();
  
  // Validate target name
  if (!IsValidTargetName(target_name)) {
    task->result_code_ = 1;
    auto* ipc_manager = CHI_IPC;
    task->error_message_ = chi::string(main_allocator_, "Invalid target name");
    // Task completion handled by framework
    return;
  }
  
  // Check if target already registered
  if (registered_targets_.find(target_name) != registered_targets_.end()) {
    task->result_code_ = 2;
    auto* ipc_manager = CHI_IPC;
    task->error_message_ = chi::string(main_allocator_, "Target already registered");
    // Task completion handled by framework
    return;
  }
  
  // Get pool name for this target (should be equivalent to target name)
  std::string pool_name = GetPoolNameForTarget(target_name);
  
  // Register target
  registered_targets_[target_name] = pool_name;
  
  task->result_code_ = 0;
}

void Runtime::MonitorRegisterTarget(chi::MonitorModeId mode, hipc::FullPtr<RegisterTargetTask> task,
                                   chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 200; // 0.2ms estimated load
      break;
    }
  }
}

void Runtime::UnregisterTarget(hipc::FullPtr<UnregisterTargetTask> task, chi::RunContext& ctx) {
  chi::ScopedCoMutex target_lock(target_mutex_);
  
  std::string target_name = task->target_name_.str();
  
  // Check if target exists
  auto it = registered_targets_.find(target_name);
  if (it == registered_targets_.end()) {
    task->result_code_ = 1;
    auto* ipc_manager = CHI_IPC;
    task->error_message_ = chi::string(main_allocator_, "Target not found");
    // Task completion handled by framework
    return;
  }
  
  // Remove target
  registered_targets_.erase(it);
  
  task->result_code_ = 0;
}

void Runtime::MonitorUnregisterTarget(chi::MonitorModeId mode, hipc::FullPtr<UnregisterTargetTask> task,
                                     chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 200; // 0.2ms estimated load
      break;
    }
  }
}

void Runtime::ListTargets(hipc::FullPtr<ListTargetsTask> task, chi::RunContext& ctx) {
  chi::ScopedCoMutex target_lock(target_mutex_);
  
  // Clear and resize the targets vector
  task->targets_.clear();
  task->targets_.reserve(registered_targets_.size());
  
  // Copy all target names
  for (const auto& pair : registered_targets_) {
    chi::string target_str(task->targets_.GetAllocator(), pair.first.c_str());
    task->targets_.emplace_back(std::move(target_str));
  }
  
  task->result_code_ = 0;
}

void Runtime::MonitorListTargets(chi::MonitorModeId mode, hipc::FullPtr<ListTargetsTask> task,
                                chi::RunContext& ctx) {
  switch (mode) {
    case chi::MonitorModeId::kEstLoad: {
      ctx.estimated_completion_time_us = 500; // 0.5ms estimated load
      break;
    }
  }
}

// ==============================================================================
// Helper Methods
// ==============================================================================

void Runtime::UpdatePerformanceMetrics(bool is_read, chi::u64 bytes, double time_us) {
  chi::ScopedCoMutex perf_lock(perf_mutex_);
  
  if (is_read) {
    total_reads_++;
    total_read_time_ += time_us;
    perf_metrics_.read_latency_us_ = total_read_time_ / total_reads_;
    perf_metrics_.read_bandwidth_mbps_ = (bytes / (1024.0 * 1024.0)) / (time_us / 1000000.0);
  } else {
    total_writes_++;
    total_write_time_ += time_us;
    perf_metrics_.write_latency_us_ = total_write_time_ / total_writes_;
    perf_metrics_.write_bandwidth_mbps_ = (bytes / (1024.0 * 1024.0)) / (time_us / 1000000.0);
  }
  
  // Calculate IOPS
  double total_ops = total_reads_ + total_writes_;
  double total_time_s = (total_read_time_ + total_write_time_) / 1000000.0;
  if (total_time_s > 0) {
    perf_metrics_.iops_ = total_ops / total_time_s;
  }
}

bool Runtime::IsValidTargetName(const std::string& target_name) const {
  // Target name validation:
  // - Must not be empty
  // - Must contain only alphanumeric characters, hyphens, and underscores
  // - Must start with alphanumeric character
  
  if (target_name.empty()) return false;
  
  if (!std::isalnum(target_name[0])) return false;
  
  for (char c : target_name) {
    if (!std::isalnum(c) && c != '-' && c != '_') {
      return false;
    }
  }
  
  return true;
}

std::string Runtime::GetPoolNameForTarget(const std::string& target_name) const {
  // According to the requirements, target name and bdev pool name should be equivalent
  return target_name;
}

// ==============================================================================
// Container Virtual Function Implementations
// ==============================================================================

chi::u64 Runtime::GetWorkRemaining() const {
  // Return approximate work remaining (simple implementation)
  // In a real implementation, this would sum tasks across all queues
  return 0; // For now, always return 0 work remaining
}

}  // namespace chimaera::bdev_extended