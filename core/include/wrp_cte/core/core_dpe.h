#ifndef WRPCTE_CORE_DPE_H_
#define WRPCTE_CORE_DPE_H_

#include <chimaera/chimaera.h>
#include <wrp_cte/core/core_tasks.h>
#include <vector>
#include <string>
#include <memory>
#include <random>

namespace wrp_cte::core {

/**
 * Data Placement Engine types
 */
enum class DpeType : chi::u32 {
  kRandom = 0,    // Random placement
  kRoundRobin = 1, // Round-robin placement
  kMaxBW = 2      // Max bandwidth placement
};

/**
 * Convert DPE type string to enum
 */
DpeType StringToDpeType(const std::string& dpe_str);

/**
 * Convert DPE type enum to string
 */
std::string DpeTypeToString(DpeType dpe_type);

/**
 * Abstract Data Placement Engine interface
 */
class DataPlacementEngine {
public:
  virtual ~DataPlacementEngine() = default;
  
  /**
   * Select targets for data placement
   * @param targets Available targets for placement
   * @param blob_score Score of the blob (0-1)
   * @param data_size Size of data to be placed
   * @return Vector of ordered targets, empty if no suitable targets
   */
  virtual std::vector<TargetInfo> SelectTargets(const std::vector<TargetInfo>& targets, 
                                               float blob_score, 
                                               chi::u64 data_size) = 0;
  
  /**
   * Get the DPE type
   */
  virtual DpeType GetType() const = 0;
};

/**
 * Random Data Placement Engine
 */
class RandomDpe : public DataPlacementEngine {
public:
  RandomDpe();
  
  std::vector<TargetInfo> SelectTargets(const std::vector<TargetInfo>& targets, 
                                       float blob_score, 
                                       chi::u64 data_size) override;
  
  DpeType GetType() const override { return DpeType::kRandom; }

private:
  std::mt19937 rng_;
};

/**
 * Round-Robin Data Placement Engine
 */
class RoundRobinDpe : public DataPlacementEngine {
public:
  RoundRobinDpe();
  
  std::vector<TargetInfo> SelectTargets(const std::vector<TargetInfo>& targets, 
                                       float blob_score, 
                                       chi::u64 data_size) override;
  
  DpeType GetType() const override { return DpeType::kRoundRobin; }

private:
  static std::atomic<chi::u32> round_robin_counter_;
};

/**
 * Max Bandwidth Data Placement Engine
 */
class MaxBwDpe : public DataPlacementEngine {
public:
  MaxBwDpe();
  
  std::vector<TargetInfo> SelectTargets(const std::vector<TargetInfo>& targets, 
                                       float blob_score, 
                                       chi::u64 data_size) override;
  
  DpeType GetType() const override { return DpeType::kMaxBW; }

private:
  static constexpr chi::u64 kLatencyThreshold = 32 * 1024; // 32KB threshold
};

/**
 * Data Placement Engine Factory
 */
class DpeFactory {
public:
  /**
   * Create a DPE instance
   * @param dpe_type Type of DPE to create
   * @return Unique pointer to DPE instance
   */
  static std::unique_ptr<DataPlacementEngine> CreateDpe(DpeType dpe_type);
  
  /**
   * Create a DPE instance from string
   * @param dpe_str DPE type as string
   * @return Unique pointer to DPE instance
   */
  static std::unique_ptr<DataPlacementEngine> CreateDpe(const std::string& dpe_str);
};

} // namespace wrp_cte::core

#endif // WRPCTE_CORE_DPE_H_