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

#include "adapter/cae_config.h"
#include "wrp_cte/core/content_transfer_engine.h"
#include "hermes_shm/util/logging.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace wrp::cae {

// Define global pointer variable in source file
HSHM_DEFINE_GLOBAL_PTR_VAR_CC(wrp::cae::CaeConfig, g_cae_config);

bool CaeConfig::LoadFromFile(const std::string &config_path) {
  if (config_path.empty()) {
    HILOG(kWarning, "Empty config path provided for CAE configuration");
    return false;
  }

  if (!std::filesystem::exists(config_path)) {
    HILOG(kWarning, "CAE config file does not exist: {}", config_path);
    return false;
  }

  try {
    YAML::Node config = YAML::LoadFile(config_path);
    return LoadFromYaml(config);
  } catch (const YAML::Exception &e) {
    HELOG(kError, "Failed to load CAE config from file {}: {}", config_path,
          e.what());
    return false;
  }
}

bool CaeConfig::LoadFromString(const std::string &yaml_content) {
  if (yaml_content.empty()) {
    HILOG(kWarning, "Empty YAML content provided for CAE configuration");
    return false;
  }

  try {
    YAML::Node config = YAML::Load(yaml_content);
    return LoadFromYaml(config);
  } catch (const YAML::Exception &e) {
    HELOG(kError, "Failed to load CAE config from YAML string: {}", e.what());
    return false;
  }
}

bool CaeConfig::LoadFromYaml(const YAML::Node &config) {
  try {
    // Load tracked paths
    if (config["paths"]) {
      paths_.clear();
      const auto &paths_node = config["paths"];
      if (paths_node.IsSequence()) {
        for (const auto &path_node : paths_node) {
          if (path_node.IsScalar()) {
            std::string path = path_node.as<std::string>();
            if (!path.empty()) {
              paths_.push_back(path);
            }
          }
        }
      } else {
        HELOG(kError, "CAE config 'paths' must be a sequence");
        return false;
      }
    }

    // Load adapter page size
    if (config["adapter_page_size"]) {
      adapter_page_size_ = config["adapter_page_size"].as<size_t>();
      if (adapter_page_size_ == 0) {
        HILOG(kWarning, "Invalid adapter page size 0, using default 4096");
        adapter_page_size_ = 4096;
      }
    }

    // Load interception enabled setting
    if (config["interception_enabled"]) {
      interception_enabled_ = config["interception_enabled"].as<bool>();
    }

    HILOG(kInfo,
          "CAE config loaded: {} tracked paths, page size {} bytes, "
          "interception {}",
          paths_.size(), adapter_page_size_,
          interception_enabled_ ? "enabled" : "disabled");
    return true;

  } catch (const YAML::Exception &e) {
    HELOG(kError, "Failed to parse CAE config YAML: {}", e.what());
    return false;
  }
}

bool CaeConfig::SaveToFile(const std::string &config_path) const {
  if (config_path.empty()) {
    HELOG(kError, "Empty config path provided for saving CAE configuration");
    return false;
  }

  try {
    // Create parent directory if it doesn't exist
    std::filesystem::path file_path(config_path);
    if (file_path.has_parent_path()) {
      std::filesystem::create_directories(file_path.parent_path());
    }

    std::ofstream file(config_path);
    if (!file.is_open()) {
      HELOG(kError, "Failed to open CAE config file for writing: {}",
            config_path);
      return false;
    }

    file << ToYamlString();
    file.close();

    HILOG(kInfo, "CAE config saved to: {}", config_path);
    return true;

  } catch (const std::exception &e) {
    HELOG(kError, "Failed to save CAE config to file {}: {}", config_path,
          e.what());
    return false;
  }
}

std::string CaeConfig::ToYamlString() const {
  YAML::Node config;

  // Add tracked paths
  config["paths"] = paths_;

  // Add adapter page size
  config["adapter_page_size"] = adapter_page_size_;

  // Add interception enabled setting
  config["interception_enabled"] = interception_enabled_;

  YAML::Emitter emitter;
  emitter << config;

  return emitter.c_str();
}

bool CaeConfig::IsPathTracked(const std::string &path) const {
  // Check if CTE is not initialized yet
  auto *cte_manager = CTE_MANAGER;
  if (cte_manager != nullptr && !cte_manager->IsInitialized()) {
    return false;
  }

  if (paths_.empty()) {
    // If no paths are configured, track everything
    return true;
  }

  for (const auto &tracked_path : paths_) {
    // Simple prefix matching for now
    // Could be extended to support glob patterns or regex
    if (path.find(tracked_path) == 0) {
      return true;
    }
  }

  return false;
}

void CaeConfig::AddTrackedPath(const std::string &path) {
  if (path.empty()) {
    return;
  }

  // Check if path already exists
  auto it = std::find(paths_.begin(), paths_.end(), path);
  if (it == paths_.end()) {
    paths_.push_back(path);
    HILOG(kDebug, "Added tracked path: {}", path);
  }
}

void CaeConfig::RemoveTrackedPath(const std::string &path) {
  auto it = std::find(paths_.begin(), paths_.end(), path);
  if (it != paths_.end()) {
    paths_.erase(it);
    HILOG(kDebug, "Removed tracked path: {}", path);
  }
}

void CaeConfig::ClearTrackedPaths() {
  paths_.clear();
  HILOG(kDebug, "Cleared all tracked paths");
}

bool WRP_CAE_CONFIG_INIT(const std::string &config_path) {
  // Get the global configuration instance (creates it if needed)
  auto *config = WRP_CAE_CONFIG;

  // Load configuration from file if provided
  if (!config_path.empty()) {
    if (!config->LoadFromFile(config_path)) {
      HILOG(kWarning, "Failed to load CAE config from {}, using defaults",
            config_path);
    }
  } else {
    // Set some reasonable defaults if no config file is provided
    config->SetAdapterPageSize(hshm::Unit<size_t>::Megabytes(1));
    config->AddTrackedPath("/tmp"); // Default to tracking /tmp directory
    HILOG(kInfo, "CAE config initialized with defaults");
  }

  return true;
}

} // namespace wrp::cae