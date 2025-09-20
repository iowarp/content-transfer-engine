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

#ifndef WRP_CTE_ADAPTER_CTE_CONFIG_H_
#define WRP_CTE_ADAPTER_CTE_CONFIG_H_

#include <string>
#include <vector>

#include "adapter/adapter_types.h"

// MDM operation constants
const int kMDM_Create = 1;
const int kMDM_Update = 2;  
const int kMDM_Delete = 3;
const int kMDM_Find = 4;
const int kMDM_Find2 = 5;

// Forward declaration for configuration  
namespace config {
  struct UserPathInfo {
    std::string path_;
    bool is_directory_ = false;
    bool include_ = true;
    
    bool Match(const std::string& abs_path) {
      (void)abs_path;
      return true; // Simple mock - all paths match for CTE
    }
  };
}

struct MockConfig {
  bool is_initialized_ = true;
  std::vector<config::UserPathInfo> path_list_;
  
  wrp::cae::AdapterObjectConfig GetAdapterConfig(const std::string& path) {
    (void)path;
    return {wrp::cae::AdapterMode::kDefault, 4096};
  }
  wrp::cae::AdapterMode GetBaseAdapterMode() {
    return wrp::cae::AdapterMode::kDefault;
  }
};

extern MockConfig g_hermes_client_conf;
extern MockConfig g_hermes_conf;

#define WRP_CTE_CLIENT_CONF (g_hermes_client_conf)
#define WRP_CTE_CONF (&g_hermes_conf)

#endif  // WRP_CTE_ADAPTER_CTE_CONFIG_H_