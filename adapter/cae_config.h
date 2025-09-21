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

#ifndef WRP_CAE_CONFIG_H_
#define WRP_CAE_CONFIG_H_

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <hermes_shm/util/singleton.h>

namespace wrp::cae {

/**
 * Configuration structure for Content Adapter Engine (CAE)
 * Contains paths to track and adapter-specific settings
 */
struct CaeConfig {
public:
  std::vector<std::string> paths_;        // Paths to track for adapter interception
  size_t adapter_page_size_;              // Page size for adapter operations (bytes)
  bool interception_enabled_;             // Enable/disable interception
  
  // Default constructor
  CaeConfig() : adapter_page_size_(4096), interception_enabled_(true) {}
  
  /**
   * Load configuration from YAML file
   * @param config_path Path to YAML configuration file
   * @return true if loaded successfully, false otherwise
   */
  bool LoadFromFile(const std::string& config_path);
  
  /**
   * Load configuration from YAML string
   * @param yaml_content YAML content as string
   * @return true if loaded successfully, false otherwise
   */
  bool LoadFromString(const std::string& yaml_content);
  
  /**
   * Save configuration to YAML file
   * @param config_path Path to save YAML configuration file
   * @return true if saved successfully, false otherwise
   */
  bool SaveToFile(const std::string& config_path) const;
  
  /**
   * Convert configuration to YAML string
   * @return YAML representation as string
   */
  std::string ToYamlString() const;
  
  /**
   * Check if a path should be tracked by adapters
   * @param path Path to check
   * @return true if path matches any tracked pattern, false otherwise
   */
  bool IsPathTracked(const std::string& path) const;
  
  /**
   * Add a path to track
   * @param path Path pattern to add
   */
  void AddTrackedPath(const std::string& path);
  
  /**
   * Remove a path from tracking
   * @param path Path pattern to remove
   */
  void RemoveTrackedPath(const std::string& path);
  
  /**
   * Clear all tracked paths
   */
  void ClearTrackedPaths();
  
  /**
   * Get the adapter page size
   * @return Page size in bytes
   */
  size_t GetAdapterPageSize() const { return adapter_page_size_; }
  
  /**
   * Set the adapter page size
   * @param page_size Page size in bytes
   */
  void SetAdapterPageSize(size_t page_size) { adapter_page_size_ = page_size; }
  
  /**
   * Get list of tracked paths
   * @return Vector of tracked path patterns
   */
  const std::vector<std::string>& GetTrackedPaths() const { return paths_; }
  
  /**
   * Check if interception is enabled
   * @return true if interception is enabled, false otherwise
   */
  bool IsInterceptionEnabled() const { return interception_enabled_; }
  
  /**
   * Enable interception
   */
  void EnableInterception() { interception_enabled_ = true; }
  
  /**
   * Disable interception
   */
  void DisableInterception() { interception_enabled_ = false; }

private:
  /**
   * Load configuration from YAML node
   * @param config YAML node containing configuration
   * @return true if loaded successfully, false otherwise
   */
  bool LoadFromYaml(const YAML::Node& config);
};

// Global pointer-based singleton with lazy initialization
HSHM_DEFINE_GLOBAL_PTR_VAR_H(wrp::cae::CaeConfig, g_cae_config);

/**
 * Initialize CAE configuration subsystem
 * @param config_path Optional path to configuration file
 * @return true if initialization succeeded, false otherwise
 */
bool WRP_CAE_CONFIG_INIT(const std::string& config_path = "");

}  // namespace wrp::cae

// Global singleton macro for easy access
#define WRP_CAE_CONFIG (HSHM_GET_GLOBAL_PTR_VAR(wrp::cae::CaeConfig, wrp::cae::g_cae_config))

#endif  // WRP_CAE_CONFIG_H_