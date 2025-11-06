#ifndef WRPCTE_CORE_CONFIG_H_
#define WRPCTE_CORE_CONFIG_H_

#include <chimaera/chimaera.h>
#include <yaml-cpp/yaml.h>
#include <hermes_shm/util/config_parse.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace wrp_cte::core {

// Named queue priorities for semantic clarity
constexpr chi::QueueId kLowLatencyQueue = 0;
constexpr chi::QueueId kHighLatencyQueue = 1;

/**
 * Queue configuration structure for CTE Core
 */
struct QueueConfig {
  chi::u32 lane_count_;
  chi::QueueId queue_id_;

  QueueConfig() : lane_count_(1), queue_id_(kLowLatencyQueue) {}
  QueueConfig(chi::u32 lane_count, chi::QueueId queue_id)
      : lane_count_(lane_count), queue_id_(queue_id) {}
};

/**
 * Performance configuration for CTE Core operations
 */
struct PerformanceConfig {
  chi::u32 target_stat_interval_ms_;    // Interval for updating target stats
  chi::u32 blob_cache_size_mb_;         // Cache size for blob operations
  chi::u32 max_concurrent_operations_;  // Max concurrent I/O operations
  float score_threshold_;               // Threshold for blob reorganization
  float score_difference_threshold_;    // Minimum score difference for reorganization

  PerformanceConfig()
      : target_stat_interval_ms_(5000),
        blob_cache_size_mb_(256),
        max_concurrent_operations_(64),
        score_threshold_(0.7f),
        score_difference_threshold_(0.05f) {}
};

/**
 * Target management configuration
 */
struct TargetConfig {
  chi::u32 neighborhood_;               // Number of targets (nodes CTE can buffer to)
  chi::u32 default_target_timeout_ms_;  // Default timeout for target operations
  chi::u32 poll_period_ms_;             // Period to rescan targets for statistics

  TargetConfig()
      : neighborhood_(4),
        default_target_timeout_ms_(30000),
        poll_period_ms_(5000) {}
};

/**
 * Storage block device configuration entry
 */
struct StorageDeviceConfig {
  std::string path_;          // Directory path for the block device
  std::string bdev_type_;     // Block device type ("file", "ram", etc.)
  chi::u64 capacity_limit_;   // Capacity limit in bytes (parsed from size string)
  float score_;               // Optional manual score (0.0-1.0), -1.0 means use automatic scoring
  
  StorageDeviceConfig() : capacity_limit_(0), score_(-1.0f) {}
  StorageDeviceConfig(const std::string& path, const std::string& bdev_type, chi::u64 capacity, float score = -1.0f)
      : path_(path), bdev_type_(bdev_type), capacity_limit_(capacity), score_(score) {}
};

/**
 * Storage configuration section
 */
struct StorageConfig {
  std::vector<StorageDeviceConfig> devices_;  // List of storage devices
  
  StorageConfig() = default;
};

/**
 * Data Placement Engine configuration
 */
struct DpeConfig {
  std::string dpe_type_;  // DPE algorithm type ("random", "round_robin", "max_bw")
  
  DpeConfig() : dpe_type_("random") {}
  explicit DpeConfig(const std::string& dpe_type) : dpe_type_(dpe_type) {}
};

/**
 * CTE Core Configuration Manager
 * Provides YAML parsing and validation for CTE Core configuration
 */
class Config {
 public:
  /**
   * Worker thread configuration
   */
  chi::u32 worker_count_;
  
  /**
   * Queue configurations for different operation types
   */
  QueueConfig target_management_queue_;
  QueueConfig tag_management_queue_;
  QueueConfig blob_operations_queue_;
  QueueConfig stats_queue_;
  
  /**
   * Performance settings
   */
  PerformanceConfig performance_;
  
  /**
   * Target management settings
   */
  TargetConfig targets_;
  
  /**
   * Storage configuration
   */
  StorageConfig storage_;
  
  /**
   * Data Placement Engine configuration
   */
  DpeConfig dpe_;
  
  /**
   * Environment variable configuration
   */
  std::string config_env_var_;
  
  /**
   * Default constructor
   */
  Config() : worker_count_(4),
             target_management_queue_(2, kLowLatencyQueue),
             tag_management_queue_(2, kLowLatencyQueue),
             blob_operations_queue_(4, kHighLatencyQueue),
             stats_queue_(1, kHighLatencyQueue),
             config_env_var_("WRP_CTE_CONF") {}

  /**
   * Constructor with allocator (for compatibility)
   */
  explicit Config(void *alloc)
      : worker_count_(4),
        target_management_queue_(2, kLowLatencyQueue),
        tag_management_queue_(2, kLowLatencyQueue),
        blob_operations_queue_(4, kHighLatencyQueue),
        stats_queue_(1, kHighLatencyQueue),
        config_env_var_("WRP_CTE_CONF") {
    (void)alloc; // Suppress unused variable warning
  }
  
  /**
   * Load configuration from YAML file
   * @param config_file_path Path to YAML configuration file
   * @return true if successful, false otherwise
   */
  bool LoadFromFile(const std::string &config_file_path);
  
  /**
   * Load configuration from environment variables
   * Falls back to config file specified in environment variable
   * @return true if successful, false otherwise
   */
  bool LoadFromEnvironment();
  
  /**
   * Save configuration to YAML file
   * @param config_file_path Path to output YAML file
   * @return true if successful, false otherwise
   */
  bool SaveToFile(const std::string &config_file_path) const;
  
  /**
   * Validate configuration parameters
   * @return true if configuration is valid, false otherwise
   */
  bool Validate() const;
  
  /**
   * Get configuration parameter as string for debugging
   * @param param_name Parameter name
   * @return Parameter value as string, empty if not found
   */
  std::string GetParameterString(const std::string &param_name) const;
  
  /**
   * Set configuration parameter from string
   * @param param_name Parameter name
   * @param value Parameter value as string
   * @return true if successful, false if parameter not found or invalid
   */
  bool SetParameterFromString(const std::string &param_name, 
                              const std::string &value);
  
 protected:
  /**
   * Parse YAML node and populate configuration
   * @param node YAML node to parse
   * @return true if successful, false otherwise
   */
  bool ParseYamlNode(const YAML::Node &node);
  
  /**
   * Generate YAML representation of configuration
   * @param emitter YAML emitter to populate with configuration data
   */
  void EmitYaml(YAML::Emitter &emitter) const;
  
 private:
  /**
   * Parse queue configuration from YAML
   * @param node YAML node containing queue config
   * @param queue_config Output queue configuration
   * @return true if successful, false otherwise
   */
  bool ParseQueueConfig(const YAML::Node &node, QueueConfig &queue_config);
  
  /**
   * Parse performance configuration from YAML
   * @param node YAML node containing performance config
   * @return true if successful, false otherwise
   */
  bool ParsePerformanceConfig(const YAML::Node &node);
  
  /**
   * Parse target configuration from YAML
   * @param node YAML node containing target config
   * @return true if successful, false otherwise
   */
  bool ParseTargetConfig(const YAML::Node &node);
  
  /**
   * Parse storage configuration from YAML
   * @param node YAML node containing storage config
   * @return true if successful, false otherwise
   */
  bool ParseStorageConfig(const YAML::Node &node);
  
  /**
   * Parse DPE configuration from YAML
   * @param node YAML node containing DPE config
   * @return true if successful, false otherwise
   */
  bool ParseDpeConfig(const YAML::Node &node);
  
  /**
   * Emit queue configuration to YAML
   * @param emitter YAML emitter
   * @param name Queue configuration name
   * @param config Queue configuration
   */
  void EmitQueueConfig(YAML::Emitter &emitter, 
                       const std::string &name,
                       const QueueConfig &config) const;
  
  /**
   * Convert queue ID to string
   * @param queue_id Queue identifier
   * @return Queue ID name as string
   */
  std::string QueueIdToString(chi::QueueId queue_id) const;

  /**
   * Convert string to queue ID
   * @param queue_str Queue ID name as string
   * @return Queue ID value, or kLowLatencyQueue if not found
   */
  chi::QueueId StringToQueueId(const std::string &queue_str) const;
  
  /**
   * Validate queue configuration
   * @param config Queue configuration to validate
   * @param queue_name Queue name for error messages
   * @return true if valid, false otherwise
   */
  bool ValidateQueueConfig(const QueueConfig &config, 
                           const std::string &queue_name) const;
  
  /**
   * Parse size string to bytes (e.g., "1GB", "512MB", "2TB")
   * @param size_str Size string to parse
   * @param size_bytes Output size in bytes
   * @return true if successful, false otherwise
   */
  bool ParseSizeString(const std::string &size_str, chi::u64 &size_bytes) const;
  
  /**
   * Format size in bytes to human-readable string (e.g., "1GB", "512MB")
   * @param size_bytes Size in bytes
   * @return Formatted size string
   */
  std::string FormatSizeBytes(chi::u64 size_bytes) const;
};

/**
 * Configuration manager singleton
 * Provides global access to CTE Core configuration
 */
class ConfigManager {
 public:
  /**
   * Get singleton instance
   * @return Reference to configuration manager instance
   */
  static ConfigManager& GetInstance();
  
  /**
   * Initialize configuration manager with allocator
   * @param alloc Allocator for configuration data (unused in current implementation)
   */
  void Initialize(void *alloc);
  
  /**
   * Load configuration from file
   * @param config_file_path Path to YAML configuration file
   * @return true if successful, false otherwise
   */
  bool LoadConfig(const std::string &config_file_path);
  
  /**
   * Load configuration from environment
   * @return true if successful, false otherwise
   */
  bool LoadConfigFromEnvironment();
  
  /**
   * Get current configuration
   * @return Reference to current configuration
   */
  const Config& GetConfig() const;
  
  /**
   * Get mutable configuration (for runtime updates)
   * @return Reference to mutable configuration
   */
  Config& GetMutableConfig();
  
  /**
   * Check if configuration is loaded and valid
   * @return true if configuration is ready, false otherwise
   */
  bool IsConfigurationReady() const;
  
 private:
  ConfigManager() = default;
  ~ConfigManager() = default;
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;
  
  void *allocator_;
  std::unique_ptr<Config> config_;
  bool initialized_;
  bool config_loaded_;
};

}  // namespace wrp_cte::core

#endif  // WRPCTE_CORE_CONFIG_H_