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

#ifndef WRP_CTE_ADAPTER_FILESYSTEM_FILESYSTEM_H_
#define WRP_CTE_ADAPTER_FILESYSTEM_FILESYSTEM_H_

#ifndef O_TMPFILE
#define O_TMPFILE 0x0
#endif

#include <ftw.h>
// #include <mpi.h>

#include <filesystem>
#include <future>
#include <set>
#include <string>

#include "filesystem_io_client.h"
#include "filesystem_mdm.h"
#include "chimaera/core/core_client.h"
#include "chimaera/core/core_tasks.h"
#include "chimaera/chimaera.h"
#include "adapter/adapter_types.h"
#include "adapter/mapper/mapper_factory.h"
#include "adapter/cae_config.h"

namespace wrp::cae {

/** The maximum length of a posix path */
static inline const int kMaxPathLen = 4096;

/** The type of seek to perform */
enum class SeekMode {
  kNone = -1,
  kSet = SEEK_SET,
  kCurrent = SEEK_CUR,
  kEnd = SEEK_END
};

/** A class to represent file system */
class Filesystem : public FilesystemIoClient {
public:
  AdapterType type_;

public:
  /** Constructor */
  explicit Filesystem(AdapterType type) : type_(type) {
    wrp_cte::core::WRP_CTE_INIT();
    wrp::cae::WRP_CAE_CONFIG_INIT();
  }

  /** open \a path */
  File Open(AdapterStat &stat, const std::string &path) {
    File f;
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    if (stat.adapter_mode_ == AdapterMode::kNone) {
      stat.adapter_mode_ = mdm->GetAdapterMode(path);
    }
    RealOpen(f, stat, path);
    if (!f.status_) {
      return f;
    }
    Open(stat, f, path);
    return f;
  }

  /** open \a f File in \a path */
  void Open(AdapterStat &stat, File &f, const std::string &path) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    // No longer need Context object for CTE

    std::shared_ptr<AdapterStat> exists = mdm->Find(f);
    if (!exists) {
      HILOG(kDebug, "File not opened before by adapter");
      // Normalize path strings
      stat.path_ = stdfs::absolute(path).string();
      // CTE uses standard strings, no need for chi::string conversion
      // CTE will create tags on demand, no need to verify existence
      // Tag creation is handled in GetOrCreateTag below
      // Update page size
      stat.page_size_ = mdm->GetAdapterPageSize(path);
      // CTE doesn't use BinaryFileStager parameters
      // Page size is managed internally by the CTE runtime
      // Initialize CTE core client and get or create tag
      // Use singleton client that should be configured globally
      
      // Get or create the tag for this file
      wrp_cte::core::TagInfo tag_info = WRP_CTE_CLIENT.GetOrCreateTag(
          hipc::MemContext(),
          stat.path_,
          wrp_cte::core::TagId::GetNull()  // Let CTE assign the ID
      );
      stat.tag_id_ = tag_info.tag_id_;
      
      if (stat.hflags_.Any(WRP_CTE_FS_TRUNC)) {
        // The file was opened with TRUNCATION
        // In CTE, we handle truncation differently - no explicit clear needed
        stat.file_size_ = 0;
      } else {
        // The file was opened regularly
        stat.file_size_ = GetBackendSize(stat.path_);
      }
      HILOG(kDebug, "Tag vs file size: tag_id={},{}, file_size={}",
            stat.tag_id_.major_, stat.tag_id_.minor_, stat.file_size_);
      // Update file position pointer
      if (stat.hflags_.Any(WRP_CTE_FS_APPEND)) {
        stat.st_ptr_ = std::numeric_limits<size_t>::max();
      } else {
        stat.st_ptr_ = 0;
      }
      // Allocate internal hermes data
      auto stat_ptr = std::make_shared<AdapterStat>(stat);
      FilesystemIoClientState fs_ctx(&mdm->fs_mdm_, (void *)stat_ptr.get());
      HermesOpen(f, stat, fs_ctx);
      mdm->Create(f, stat_ptr);
    } else {
      HILOG(kDebug, "File already opened by adapter");
      exists->UpdateTime();
    }
  }

  /** write */
  size_t Write(File &f, AdapterStat &stat, const void *ptr, size_t off,
               size_t total_size, IoStatus &io_status,
               FsIoOptions opts = FsIoOptions()) {
    (void)f;
    std::string filename = stat.path_;
    bool is_append = stat.st_ptr_ == std::numeric_limits<size_t>::max();

    // HILOG(kInfo,
    //       "Write called for filename: {}"
    //       " on offset: {}"
    //       " from position: {}"
    //       " and size: {}"
    //       " and adapter mode: {}",
    //       filename, off, stat.st_ptr_, total_size,
    //       AdapterModeConv::str(stat.adapter_mode_));
    if (stat.adapter_mode_ == AdapterMode::kBypass) {
      // Bypass mode is handled differently
      opts.backend_size_ = total_size;
      opts.backend_off_ = off;
      WriteBlob(filename, ptr, total_size, opts, io_status);
      if (!io_status.success_) {
        HILOG(kDebug, "Failed to write blob of size {} to backend",
              opts.backend_size_);
        return 0;
      }
      if (opts.DoSeek() && !is_append) {
        stat.st_ptr_ = off + total_size;
      }
      return total_size;
    }
    // CTE doesn't need Context objects

    if (is_append) {
      // TODO: Append operations not yet supported in CTE
      // Perform append
      HILOG(kWarning, "Append operations not yet supported in CTE, treating as regular write");
      // Fallback to regular write at end of file
      off = stat.file_size_;
    }
    
    // Use CTE PutBlob for all write operations
    {
      // Allocate buffer in shared memory for CTE
      hipc::FullPtr<void> blob_data = CHI_IPC->AllocateBuffer<void>(total_size);
      if (blob_data.IsNull()) {
        HILOG(kError, "Failed to allocate buffer for write operation");
        io_status.success_ = false;
        return 0;
      }
      
      // Copy data to shared memory buffer
      memcpy(blob_data.ptr_, ptr, total_size);
      
      // Generate a blob name for this write operation
      std::string blob_name = "blob_" + std::to_string(off) + "_" + std::to_string(total_size);
      
      // Use CTE PutBlob
      bool success = WRP_CTE_CLIENT.PutBlob(
          hipc::MemContext(),
          stat.tag_id_,
          blob_name,
          wrp_cte::core::BlobId::GetNull(),  // Let CTE assign blob ID
          off,                          // offset in blob
          total_size,                   // size
          blob_data.shm_,     // Convert FullPtr to Pointer using .shm_
          0.5f,                         // default score
          0                             // flags
      );
      
      // CTE manages buffer lifecycle - no explicit free needed
      
      if (!success) {
        HILOG(kError, "CTE PutBlob failed");
        io_status.success_ = false;
        return 0;
      }
      
      if (opts.DoSeek()) {
        stat.st_ptr_ = off + total_size;
      }
    }
    stat.UpdateTime();
    io_status.size_ = total_size;
    UpdateIoStatus(opts, io_status);

    HILOG(kDebug, "The size of file after write: {}", GetSize(f, stat));
    return total_size;
  }

  /** base read function */
  template <bool ASYNC>
  size_t BaseRead(File &f, AdapterStat &stat, void *ptr, size_t off,
                  size_t total_size, size_t req_id,
                  std::vector<GetBlobAsyncTask> &tasks, IoStatus &io_status,
                  FsIoOptions opts = FsIoOptions()) {
    (void)f;
    std::string filename = stat.path_;

    HILOG(kDebug,
          "Read called for filename: {}"
          " on offset: {}"
          " from position: {}"
          " and size: {}",
          stat.path_, off, stat.st_ptr_, total_size);

    // SEEK_END is not a valid read position
    if (off == std::numeric_limits<size_t>::max()) {
      io_status.size_ = 0;
      UpdateIoStatus(opts, io_status);
      return 0;
    }

    // Read bit must be set
    if (!stat.hflags_.Any(WRP_CTE_FS_READ)) {
      io_status.size_ = 0;
      UpdateIoStatus(opts, io_status);
      return -1;
    }

    // Ensure the amount being read makes sense
    if (total_size == 0) {
      io_status.size_ = 0;
      UpdateIoStatus(opts, io_status);
      return 0;
    }

    if constexpr (!ASYNC) {
      if (stat.adapter_mode_ == AdapterMode::kBypass) {
        // Bypass mode is handled differently
        opts.backend_size_ = total_size;
        opts.backend_off_ = off;
        ReadBlob(filename, ptr, total_size, opts, io_status);
        if (!io_status.success_) {
          HILOG(kDebug, "Failed to read blob of size {} from backend",
                opts.backend_size_);
          return 0;
        }
        if (opts.DoSeek()) {
          stat.st_ptr_ = off + total_size;
        }
        return total_size;
      }
    }

    // CTE read operation - simplified without page fragmentation for now
    // TODO: Implement proper blob name resolution based on offset
    std::string blob_name = "blob_" + std::to_string(off) + "_" + std::to_string(total_size);
    
    if constexpr (ASYNC) {
      // TODO: Async read operations not yet fully supported in CTE adapter
      HILOG(kWarning, "Async read operations not yet fully supported, using sync read");
    }
    
    // Allocate buffer for reading
    hipc::FullPtr<void> read_buffer = CHI_IPC->AllocateBuffer<void>(total_size);
    if (read_buffer.IsNull()) {
      HILOG(kError, "Failed to allocate buffer for read operation");
      io_status.success_ = false;
      return 0;
    }
    
    // Use CTE GetBlob
    bool success = WRP_CTE_CLIENT.GetBlob(
        hipc::MemContext(),
        stat.tag_id_,
        blob_name,
        wrp_cte::core::BlobId::GetNull(),  // Let CTE find by name
        off,                          // offset in blob
        total_size,                   // size
        0,                            // flags
        read_buffer.shm_ // Convert FullPtr to Pointer using .shm_
    );
    
    if (success) {
      // Copy data from shared memory buffer to user buffer
      memcpy(ptr, read_buffer.ptr_, total_size);
    } else {
      HILOG(kError, "CTE GetBlob failed");
      io_status.success_ = false;
      return 0;
    }
    
    size_t data_offset = total_size;  // Full read completed
    if (opts.DoSeek()) {
      stat.st_ptr_ = off + data_offset;
    }
    stat.UpdateTime();
    io_status.size_ = data_offset;
    UpdateIoStatus(opts, io_status);
    return data_offset;
  }

  /** read */
  size_t Read(File &f, AdapterStat &stat, void *ptr, size_t off,
              size_t total_size, IoStatus &io_status,
              FsIoOptions opts = FsIoOptions()) {
    std::vector<GetBlobAsyncTask> tasks;
    return BaseRead<false>(f, stat, ptr, off, total_size, 0, tasks, io_status,
                           opts);
  }

  /** write asynchronously */
  FsAsyncTask *AWrite(File &f, AdapterStat &stat, const void *ptr, size_t off,
                      size_t total_size, size_t req_id, IoStatus &io_status,
                      FsIoOptions opts = FsIoOptions()) {
    // Writes are completely async at this time
    FsAsyncTask *fstask = new FsAsyncTask();
    Write(f, stat, ptr, off, total_size, io_status, opts);
    fstask->io_status_.Copy(io_status);
    fstask->opts_ = opts;
    return fstask;
  }

  /** read asynchronously */
  FsAsyncTask *ARead(File &f, AdapterStat &stat, void *ptr, size_t off,
                     size_t total_size, size_t req_id, IoStatus &io_status,
                     FsIoOptions opts = FsIoOptions()) {
    FsAsyncTask *fstask = new FsAsyncTask();
    BaseRead<true>(f, stat, ptr, off, total_size, req_id, fstask->get_tasks_,
                   io_status, opts);
    fstask->io_status_ = io_status;
    fstask->opts_ = opts;
    return fstask;
  }

  /** wait for \a req_id request ID */
  size_t Wait(FsAsyncTask *fstask) {
    // CTE async operations - updated for new task types
    for (hipc::FullPtr<wrp_cte::core::PutBlobTask> &task : fstask->put_tasks_) {
      task->Wait();
      CHI_IPC->DelTask(task);
    }

    // Update I/O status for gets
    if (!fstask->get_tasks_.empty()) {
      size_t get_size = 0;
      for (GetBlobAsyncTask &task : fstask->get_tasks_) {
        task.task_->Wait();
        // TODO: CTE GetBlob tasks may have different result structure
        // For now, just use the requested size
        get_size += task.orig_size_;
        // TODO: CTE may handle data copying differently
        // memcpy(task.orig_data_, data.ptr_, task.orig_size_);
        CHI_IPC->DelTask(task.task_);
      }
      fstask->io_status_.size_ = get_size;
      UpdateIoStatus(fstask->opts_, fstask->io_status_);
    }
    return 0;
  }

  /** wait for request IDs in \a req_id vector */
  void Wait(std::vector<FsAsyncTask *> &req_ids, std::vector<size_t> &ret) {
    for (auto &req_id : req_ids) {
      ret.emplace_back(Wait(req_id));
    }
  }

  /** seek */
  size_t Seek(File &f, AdapterStat &stat, SeekMode whence, off64_t offset) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    switch (whence) {
    case SeekMode::kSet: {
      stat.st_ptr_ = offset;
      break;
    }
    case SeekMode::kCurrent: {
      if (stat.st_ptr_ != std::numeric_limits<size_t>::max()) {
        stat.st_ptr_ = (off64_t)stat.st_ptr_ + offset;
        offset = stat.st_ptr_;
      } else {
        stat.st_ptr_ = (off64_t)stat.file_size_ + offset;
        offset = stat.st_ptr_;
      }
      break;
    }
    case SeekMode::kEnd: {
      if (offset == 0) {
        stat.st_ptr_ = std::numeric_limits<size_t>::max();
        offset = stat.file_size_;
      } else {
        stat.st_ptr_ = (off64_t)stat.file_size_ + offset;
        offset = stat.st_ptr_;
      }
      break;
    }
    default: {
      HELOG(kError, "Invalid seek mode");
      return (size_t)-1;
    }
    }
    mdm->Update(f, stat);
    return offset;
  }

  /** file size */
  size_t GetSize(File &f, AdapterStat &stat) {
    (void)f;
    if (stat.adapter_mode_ != AdapterMode::kBypass) {
      // For CTE, use the cached file size
      // TODO: Could query CTE for actual tag size if needed
      return stat.file_size_;
    } else {
      return stdfs::file_size(stat.path_);
    }
  }

  /** tell */
  size_t Tell(File &f, AdapterStat &stat) {
    (void)f;
    if (stat.st_ptr_ != std::numeric_limits<size_t>::max()) {
      return stat.st_ptr_;
    } else {
      return stat.file_size_;
    }
  }

  /** sync */
  int Sync(File &f, AdapterStat &stat) {
    (void)f;
    (void)stat;
    // CTE sync operations would be handled by the runtime
    // For now, no explicit sync needed
    return 0;
  }

  /** truncate */
  int Truncate(File &f, AdapterStat &stat, size_t new_size) {
    // hapi::Bucket &bkt = stat.bkt_id_;
    // TODO(llogan)
    return 0;
  }

  /** close */
  int Close(File &f, AdapterStat &stat) {
    Sync(f, stat);
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    FilesystemIoClientState fs_ctx(&mdm->fs_mdm_, (void *)&stat);
    HermesClose(f, stat, fs_ctx);
    RealClose(f, stat);
    mdm->Delete(stat.path_, f);
    if (stat.amode_ & MPI_MODE_DELETE_ON_CLOSE) {
      Remove(stat.path_);
    }
    // CTE doesn't require explicit flush operations
    // Runtime handles persistence automatically
    return 0;
  }

  /** remove */
  int Remove(const std::string &pathname) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    int ret = RealRemove(pathname);
    // CTE tag cleanup - tags are managed by the runtime
    std::string canon_path = stdfs::absolute(pathname).string();
    // TODO: Add explicit tag cleanup if needed by CTE
    (void)canon_path;  // Suppress unused variable warning
    // Destroy all file descriptors
    std::list<File> *filesp = mdm->Find(pathname);
    if (filesp == nullptr) {
      return ret;
    }
    HILOG(kDebug, "Destroying the file descriptors: {}", pathname);
    std::list<File> files = *filesp;
    for (File &f : files) {
      std::shared_ptr<AdapterStat> stat = mdm->Find(f);
      if (stat == nullptr) {
        continue;
      }
      FilesystemIoClientState fs_ctx(&mdm->fs_mdm_, (void *)&stat);
      HermesClose(f, *stat, fs_ctx);
      RealClose(f, *stat);
      mdm->Delete(stat->path_, f);
      if (stat->adapter_mode_ == AdapterMode::kScratch) {
        ret = 0;
      }
    }
    return ret;
  }

  /**
   * I/O APIs which seek based on the internal AdapterStat st_ptr,
   * instead of taking an offset as input.
   */

public:
  /** write */
  size_t Write(File &f, AdapterStat &stat, const void *ptr, size_t total_size,
               IoStatus &io_status, FsIoOptions opts) {
    size_t off = stat.st_ptr_;
    return Write(f, stat, ptr, off, total_size, io_status, opts);
  }

  /** read */
  size_t Read(File &f, AdapterStat &stat, void *ptr, size_t total_size,
              IoStatus &io_status, FsIoOptions opts) {
    size_t off = stat.st_ptr_;
    return Read(f, stat, ptr, off, total_size, io_status, opts);
  }

  /** write asynchronously */
  FsAsyncTask *AWrite(File &f, AdapterStat &stat, const void *ptr,
                      size_t total_size, size_t req_id, IoStatus &io_status,
                      FsIoOptions opts) {
    size_t off = stat.st_ptr_;
    return AWrite(f, stat, ptr, off, total_size, req_id, io_status, opts);
  }

  /** read asynchronously */
  FsAsyncTask *ARead(File &f, AdapterStat &stat, void *ptr, size_t total_size,
                     size_t req_id, IoStatus &io_status, FsIoOptions opts) {
    size_t off = stat.st_ptr_;
    return ARead(f, stat, ptr, off, total_size, req_id, io_status, opts);
  }

  /**
   * Locates the AdapterStat data structure internally, and
   * call the underlying APIs which take AdapterStat as input.
   */

public:
  /** write */
  size_t Write(File &f, bool &stat_exists, const void *ptr, size_t total_size,
               IoStatus &io_status, FsIoOptions opts = FsIoOptions()) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    return Write(f, *stat, ptr, total_size, io_status, opts);
  }

  /** read */
  size_t Read(File &f, bool &stat_exists, void *ptr, size_t total_size,
              IoStatus &io_status, FsIoOptions opts = FsIoOptions()) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    return Read(f, *stat, ptr, total_size, io_status, opts);
  }

  /** write \a off offset */
  size_t Write(File &f, bool &stat_exists, const void *ptr, size_t off,
               size_t total_size, IoStatus &io_status,
               FsIoOptions opts = FsIoOptions()) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    opts.UnsetSeek();
    return Write(f, *stat, ptr, off, total_size, io_status, opts);
  }

  /** read \a off offset */
  size_t Read(File &f, bool &stat_exists, void *ptr, size_t off,
              size_t total_size, IoStatus &io_status,
              FsIoOptions opts = FsIoOptions()) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    opts.UnsetSeek();
    return Read(f, *stat, ptr, off, total_size, io_status, opts);
  }

  /** write asynchronously */
  FsAsyncTask *AWrite(File &f, bool &stat_exists, const void *ptr,
                      size_t total_size, size_t req_id,
                      std::vector<hipc::FullPtr<wrp_cte::core::PutBlobTask>> &tasks, IoStatus &io_status,
                      FsIoOptions opts) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    return AWrite(f, *stat, ptr, total_size, req_id, io_status, opts);
  }

  /** read asynchronously */
  FsAsyncTask *ARead(File &f, bool &stat_exists, void *ptr, size_t total_size,
                     size_t req_id, IoStatus &io_status, FsIoOptions opts) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    return ARead(f, *stat, ptr, total_size, req_id, io_status, opts);
  }

  /** write \a off offset asynchronously */
  FsAsyncTask *AWrite(File &f, bool &stat_exists, const void *ptr, size_t off,
                      size_t total_size, size_t req_id, IoStatus &io_status,
                      FsIoOptions opts) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    opts.UnsetSeek();
    return AWrite(f, *stat, ptr, off, total_size, req_id, io_status, opts);
  }

  /** read \a off offset asynchronously */
  FsAsyncTask *ARead(File &f, bool &stat_exists, void *ptr, size_t off,
                     size_t total_size, size_t req_id, IoStatus &io_status,
                     FsIoOptions opts) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return 0;
    }
    stat_exists = true;
    opts.UnsetSeek();
    return ARead(f, *stat, ptr, off, total_size, req_id, io_status, opts);
  }

  /** seek */
  size_t Seek(File &f, bool &stat_exists, SeekMode whence, size_t offset) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return (size_t)-1;
    }
    stat_exists = true;
    return Seek(f, *stat, whence, offset);
  }

  /** file sizes */
  size_t GetSize(File &f, bool &stat_exists) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return (size_t)-1;
    }
    stat_exists = true;
    return GetSize(f, *stat);
  }

  /** tell */
  size_t Tell(File &f, bool &stat_exists) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return (size_t)-1;
    }
    stat_exists = true;
    return Tell(f, *stat);
  }

  /** sync */
  int Sync(File &f, bool &stat_exists) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return -1;
    }
    stat_exists = true;
    return Sync(f, *stat);
  }

  /** truncate */
  int Truncate(File &f, bool &stat_exists, size_t new_size) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return -1;
    }
    stat_exists = true;
    return Truncate(f, *stat, new_size);
  }

  /** close */
  int Close(File &f, bool &stat_exists) {
    auto mdm = WRP_CTE_FS_METADATA_MANAGER;
    auto stat = mdm->Find(f);
    if (!stat) {
      stat_exists = false;
      return -1;
    }
    stat_exists = true;
    return Close(f, *stat);
  }

public:
  /** Whether or not \a path PATH is tracked by Hermes */
  static bool IsPathTracked(const std::string &path) {
    // Check if the CAE config singleton is available
    auto* cae_config_ptr = HSHM_GET_GLOBAL_PTR_VAR(wrp::cae::CaeConfig, wrp::cae::g_cae_config);
    if (cae_config_ptr == nullptr) {
      return false;
    }
    if (path.empty()) {
      return false;
    }
    std::string abs_path = stdfs::absolute(path).string();
    // Use the CAE config's IsPathTracked method
    return WRP_CAE_CONFIG.IsPathTracked(abs_path);
  }
};

} // namespace wrp::cae

#endif // WRP_CTE_ADAPTER_FILESYSTEM_FILESYSTEM_H_
