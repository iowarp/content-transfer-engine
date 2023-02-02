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

#ifndef HERMES_ADAPTER_POSIX_NATIVE_H_
#define HERMES_ADAPTER_POSIX_NATIVE_H_

#include <memory>

#include "adapter/filesystem/filesystem.h"
#include "adapter/filesystem/filesystem_mdm.h"
#include "posix_api_singleton_macros.h"
#include "posix_api.h"
#include "posix_io_client.h"

namespace hermes::adapter::fs {

/** A class to represent POSIX IO file system */
class PosixFs : public hermes::adapter::fs::Filesystem {
 public:
  PosixFs() : hermes::adapter::fs::Filesystem(HERMES_POSIX_IO_CLIENT,
                                              AdapterType::kPosix) {}

  /** Whether or not \a fd FILE DESCRIPTOR was generated by Hermes */
  static bool IsFdTracked(int fd) {
    if (!HERMES->IsInitialized()) {
      return false;
    }
    hermes::adapter::fs::File f;
    f.hermes_fd_ = fd;
    std::pair<AdapterStat*, bool> stat_pair =
        HERMES_FS_METADATA_MANAGER->Find(f);
    return stat_pair.second;
  }

  /** get the file name from \a fd file descriptor */
  std::string GetFilenameFromFD(int fd) {
    char proclnk[kMaxPathLen];
    char filename[kMaxPathLen];
    snprintf(proclnk, kMaxPathLen, "/proc/self/fd/%d", fd);
    size_t r = readlink(proclnk, filename, kMaxPathLen);
    filename[r] = '\0';
    return filename;
  }
};

/** Simplify access to the stateless PosixFs Singleton */
#define HERMES_POSIX_FS hermes::EasySingleton<hermes::adapter::fs::PosixFs>::GetInstance()
#define HERMES_POSIX_FS_T hermes::adapter::fs::PosixFs*

}  // namespace hermes::adapter::fs

#endif  // HERMES_ADAPTER_POSIX_NATIVE_H_