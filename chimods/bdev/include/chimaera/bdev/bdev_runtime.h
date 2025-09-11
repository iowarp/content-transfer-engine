#ifndef BDEV_RUNTIME_H_
#define BDEV_RUNTIME_H_

#include <chimaera/chimaera.h>
#include <chimaera/bdev/bdev_tasks.h>
#include <chimaera/comutex.h>
#include <chimaera/corwlock.h>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <memory>

#ifdef __linux__
#ifdef CHIMAERA_BDEV_HAS_AIO
#include <libaio.h>
#endif
#endif

namespace chimaera::bdev_extended {

/**
 * Block allocator for managing different block sizes
 */
class BlockAllocator {
 public:
  // Block size categories
  static constexpr chi::u64 kBlockSize4KB = 4 * 1024;        // 4KB blocks
  static constexpr chi::u64 kBlockSize64KB = 64 * 1024;      // 64KB blocks  
  static constexpr chi::u64 kBlockSize256KB = 256 * 1024;    // 256KB blocks
  static constexpr chi::u64 kBlockSize1MB = 1024 * 1024;     // 1MB blocks

  BlockAllocator() : total_size_(0), allocated_size_(0) {}
  
  void Initialize(chi::u64 total_size);
  Block Allocate(chi::u64 size);
  bool Free(const Block& block);
  chi::u64 GetRemainingSize() const { return total_size_ - allocated_size_; }
  
 private:
  chi::u64 total_size_;
  chi::u64 allocated_size_;
  chi::u64 next_offset_;
  
  // Free lists for each block type
  std::vector<Block> free_blocks_4kb_;
  std::vector<Block> free_blocks_64kb_;
  std::vector<Block> free_blocks_256kb_;
  std::vector<Block> free_blocks_1mb_;
  
  chi::u32 GetBlockType(chi::u64 size) const;
  chi::u64 GetBlockSizeForType(chi::u32 block_type) const;
  std::vector<Block>& GetFreeListForType(chi::u32 block_type);
};

/**
 * File I/O backend using libaio for asynchronous operations
 */
class FileBackend {
 public:
  FileBackend() : fd_(-1), ctx_(nullptr), alignment_(4096) {}
  ~FileBackend() { Cleanup(); }
  
  bool Initialize(const std::string& file_path, chi::u32 io_depth, chi::u32 alignment);
  void Cleanup();
  
  chi::u64 Write(const Block& block, const std::vector<hshm::u8>& data);
  std::vector<hshm::u8> Read(const Block& block);
  
 private:
  int fd_;
#ifdef CHIMAERA_BDEV_HAS_AIO
  io_context_t ctx_;
#else
  void* ctx_;  // Placeholder when libaio is not available
#endif
  chi::u32 alignment_;
  
#ifdef CHIMAERA_BDEV_HAS_AIO
  void* AlignedAlloc(size_t size);
  void AlignedFree(void* ptr);
#endif
};

/**
 * RAM backend using malloc for high-speed operations
 */
class RamBackend {
 public:
  RamBackend() : buffer_(nullptr), size_(0) {}
  ~RamBackend() { Cleanup(); }
  
  bool Initialize(chi::u64 total_size);
  void Cleanup();
  
  chi::u64 Write(const Block& block, const std::vector<hshm::u8>& data);
  std::vector<hshm::u8> Read(const Block& block);
  
 private:
  void* buffer_;
  chi::u64 size_;
};

/**
 * Runtime container for the bdev ChiMod
 */
class Runtime : public chi::Container {
 public:
  using CreateParams = chimaera::bdev_extended::CreateParams;  // Required for CHI_TASK_CC
  
  Runtime() = default;
  ~Runtime() override = default;

  /**
   * Initialize client for this container (REQUIRED)
   */
  void InitClient(const chi::PoolId& pool_id) {
    // Client initialization handled by framework
    (void)pool_id;
  }

  /**
   * Create the container (Method::kCreate)
   * This method both creates and initializes the container
   */
  void Create(hipc::FullPtr<CreateTask> task, chi::RunContext& ctx);

  /**
   * Monitor create progress
   */
  void MonitorCreate(chi::MonitorModeId mode, hipc::FullPtr<CreateTask> task,
                     chi::RunContext& ctx);

  // ==============================================================================
  // Block Management Operations
  // ==============================================================================

  /**
   * Allocate a block (Method::kAllocate)
   */
  void Allocate(hipc::FullPtr<AllocateTask> task, chi::RunContext& ctx);

  /**
   * Monitor allocate operation
   */
  void MonitorAllocate(chi::MonitorModeId mode, hipc::FullPtr<AllocateTask> task,
                      chi::RunContext& ctx);

  /**
   * Free a block (Method::kFree)
   */
  void Free(hipc::FullPtr<FreeTask> task, chi::RunContext& ctx);

  /**
   * Monitor free operation
   */
  void MonitorFree(chi::MonitorModeId mode, hipc::FullPtr<FreeTask> task,
                  chi::RunContext& ctx);

  // ==============================================================================
  // I/O Operations
  // ==============================================================================

  /**
   * Write data to a block (Method::kWrite)
   */
  void Write(hipc::FullPtr<WriteTask> task, chi::RunContext& ctx);

  /**
   * Monitor write operation
   */
  void MonitorWrite(chi::MonitorModeId mode, hipc::FullPtr<WriteTask> task,
                   chi::RunContext& ctx);

  /**
   * Read data from a block (Method::kRead)
   */
  void Read(hipc::FullPtr<ReadTask> task, chi::RunContext& ctx);

  /**
   * Monitor read operation
   */
  void MonitorRead(chi::MonitorModeId mode, hipc::FullPtr<ReadTask> task,
                  chi::RunContext& ctx);

  // ==============================================================================
  // Performance Monitoring
  // ==============================================================================

  /**
   * Get performance statistics (Method::kGetStats)
   */
  void GetStats(hipc::FullPtr<StatTask> task, chi::RunContext& ctx);

  /**
   * Monitor get stats operation
   */
  void MonitorGetStats(chi::MonitorModeId mode, hipc::FullPtr<StatTask> task,
                      chi::RunContext& ctx);

  // ==============================================================================
  // Target Registration Operations
  // ==============================================================================

  /**
   * Register a target (Method::kRegisterTarget)
   * Target name must equal the bdev pool name
   */
  void RegisterTarget(hipc::FullPtr<RegisterTargetTask> task, chi::RunContext& ctx);

  /**
   * Monitor register target operation
   */
  void MonitorRegisterTarget(chi::MonitorModeId mode, hipc::FullPtr<RegisterTargetTask> task,
                            chi::RunContext& ctx);

  /**
   * Unregister a target (Method::kUnregisterTarget)
   */
  void UnregisterTarget(hipc::FullPtr<UnregisterTargetTask> task, chi::RunContext& ctx);

  /**
   * Monitor unregister target operation
   */
  void MonitorUnregisterTarget(chi::MonitorModeId mode, hipc::FullPtr<UnregisterTargetTask> task,
                              chi::RunContext& ctx);

  /**
   * List all registered targets (Method::kListTargets)
   */
  void ListTargets(hipc::FullPtr<ListTargetsTask> task, chi::RunContext& ctx);

  /**
   * Monitor list targets operation
   */
  void MonitorListTargets(chi::MonitorModeId mode, hipc::FullPtr<ListTargetsTask> task,
                         chi::RunContext& ctx);

  // ==============================================================================
  // Container Virtual Function Implementations (Generated by CHI_TASK_CC)
  // ==============================================================================
  
  /**
   * Pure virtual methods - implementations are in autogen/bdev_lib_exec.cc
   */
  void Run(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr, chi::RunContext& rctx) override;
  void Monitor(chi::MonitorModeId mode, chi::u32 method, 
               hipc::FullPtr<chi::Task> task_ptr, chi::RunContext& rctx) override;
  void Del(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr) override;
  chi::u64 GetWorkRemaining() const override;
  void SaveIn(chi::u32 method, chi::TaskSaveInArchive& archive,
              hipc::FullPtr<chi::Task> task_ptr) override;
  void LoadIn(chi::u32 method, chi::TaskLoadInArchive& archive,
              hipc::FullPtr<chi::Task> task_ptr) override;
  void SaveOut(chi::u32 method, chi::TaskSaveOutArchive& archive,
               hipc::FullPtr<chi::Task> task_ptr) override;
  void LoadOut(chi::u32 method, chi::TaskLoadOutArchive& archive,
               hipc::FullPtr<chi::Task> task_ptr) override;
  void NewCopy(chi::u32 method, const hipc::FullPtr<chi::Task> &orig_task,
               hipc::FullPtr<chi::Task> &dup_task, bool deep) override;
  // Autogen-expected signatures (older API)
  void SaveIn(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  void LoadIn(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  void SaveOut(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  void LoadOut(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);
  hipc::FullPtr<chi::Task> NewCopy(chi::u32 method, hipc::FullPtr<chi::Task> task_ptr);

 private:
  // Queue ID constants (REQUIRED: Use semantic names, not raw integers)
  static const chi::QueueId kMetadataQueue = 0;
  static const chi::QueueId kAllocQueue = 1;
  static const chi::QueueId kIoQueue = 2;
  static const chi::QueueId kTargetQueue = 3;

  // Client instance removed - not needed for runtime
  
  // Storage configuration
  BdevType bdev_type_;
  std::string file_path_;
  chi::u64 total_size_;
  chi::u32 io_depth_;
  chi::u32 alignment_;
  
  // Storage backends
  std::unique_ptr<FileBackend> file_backend_;
  std::unique_ptr<RamBackend> ram_backend_;
  
  // Block allocator
  BlockAllocator block_allocator_;
  
  // Target registration storage
  // Maps target name -> bdev pool name (should be equivalent)
  std::unordered_map<std::string, std::string> registered_targets_;
  
  // Performance tracking
  PerfMetrics perf_metrics_;
  chi::u64 total_reads_;
  chi::u64 total_writes_;
  double total_read_time_;
  double total_write_time_;
  
  // Synchronization primitives for thread-safe access
  static chi::CoRwLock data_lock_;        // For data structure access
  static chi::CoMutex target_mutex_;      // For target registration operations
  static chi::CoMutex perf_mutex_;        // For performance metrics
  
  // Helper methods
  void UpdatePerformanceMetrics(bool is_read, chi::u64 bytes, double time_us);
  bool IsValidTargetName(const std::string& target_name) const;
  std::string GetPoolNameForTarget(const std::string& target_name) const;
};

}  // namespace chimaera::bdev_extended

// Define ChiMod entry points using CHI_TASK_CC macro
CHI_TASK_CC(chimaera::bdev_extended::Runtime)

#endif  // BDEV_RUNTIME_H_