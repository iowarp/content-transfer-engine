#include <wrp_cte/core/core_dpe.h>
#include <algorithm>
#include <iostream>
#include <chrono>
#include "hermes_shm/util/logging.h"

namespace wrp_cte::core {

// Static member definition for round-robin counter
std::atomic<chi::u32> RoundRobinDpe::round_robin_counter_(0);

// DPE Type conversion functions
DpeType StringToDpeType(const std::string& dpe_str) {
  if (dpe_str == "random") {
    return DpeType::kRandom;
  } else if (dpe_str == "round_robin" || dpe_str == "roundrobin") {
    return DpeType::kRoundRobin;
  } else if (dpe_str == "max_bw" || dpe_str == "maxbw") {
    return DpeType::kMaxBW;
  } else {
    HELOG(kError, "Unknown DPE type: {}, defaulting to random", dpe_str);
    return DpeType::kRandom;
  }
}

std::string DpeTypeToString(DpeType dpe_type) {
  switch (dpe_type) {
    case DpeType::kRandom:
      return "random";
    case DpeType::kRoundRobin:
      return "round_robin";
    case DpeType::kMaxBW:
      return "max_bw";
    default:
      return "random";
  }
}

// RandomDpe Implementation
RandomDpe::RandomDpe() : rng_(std::chrono::steady_clock::now().time_since_epoch().count()) {
}

std::vector<TargetInfo> RandomDpe::SelectTargets(const std::vector<TargetInfo>& targets, 
                                                float blob_score, 
                                                chi::u64 data_size) {
  std::vector<TargetInfo> result;
  
  if (targets.empty()) {
    return result;
  }

  // Filter targets with sufficient space
  for (const auto& target : targets) {
    if (target.remaining_space_ >= data_size) {
      result.push_back(target);
    }
  }

  if (result.empty()) {
    return result;  // No targets have space
  }

  // Randomly shuffle the filtered targets
  std::shuffle(result.begin(), result.end(), rng_);
  
  return result;
}

// RoundRobinDpe Implementation  
RoundRobinDpe::RoundRobinDpe() {
}

std::vector<TargetInfo> RoundRobinDpe::SelectTargets(const std::vector<TargetInfo>& targets, 
                                                    float blob_score, 
                                                    chi::u64 data_size) {
  std::vector<TargetInfo> result;
  
  if (targets.empty()) {
    return result;
  }

  // Filter targets with sufficient space
  for (const auto& target : targets) {
    if (target.remaining_space_ >= data_size) {
      result.push_back(target);
    }
  }

  if (result.empty()) {
    return result;  // No targets have space
  }

  // Shift the target vector to the left (circular rotation)
  chi::u32 counter = round_robin_counter_.fetch_add(1);
  size_t shift_amount = counter % result.size();
  
  if (shift_amount > 0) {
    std::rotate(result.begin(), result.begin() + shift_amount, result.end());
  }
  
  return result;
}

// MaxBwDpe Implementation
MaxBwDpe::MaxBwDpe() {
}

std::vector<TargetInfo> MaxBwDpe::SelectTargets(const std::vector<TargetInfo>& targets, 
                                               float blob_score, 
                                               chi::u64 data_size) {
  std::vector<TargetInfo> result;
  
  if (targets.empty()) {
    return result;
  }

  // Filter targets with sufficient space
  std::vector<TargetInfo> available_targets;
  for (const auto& target : targets) {
    if (target.remaining_space_ >= data_size) {
      available_targets.push_back(target);
    }
  }

  if (available_targets.empty()) {
    return result;  // No targets have space
  }

  // Sort targets by performance metrics
  if (data_size >= kLatencyThreshold) {
    // Sort by write bandwidth (descending)
    std::sort(available_targets.begin(), available_targets.end(),
              [](const TargetInfo& a, const TargetInfo& b) {
                return a.perf_metrics_.write_bandwidth_mbps_ > b.perf_metrics_.write_bandwidth_mbps_;
              });
  } else {
    // Sort by latency (ascending - lower is better)
    std::sort(available_targets.begin(), available_targets.end(),
              [](const TargetInfo& a, const TargetInfo& b) {
                double avg_latency_a = (a.perf_metrics_.read_latency_us_ + a.perf_metrics_.write_latency_us_) / 2.0;
                double avg_latency_b = (b.perf_metrics_.read_latency_us_ + b.perf_metrics_.write_latency_us_) / 2.0;
                return avg_latency_a < avg_latency_b;
              });
  }

  // Filter out targets that have too high of a score
  for (const auto& target : available_targets) {
    if (target.target_score_ <= blob_score) {
      result.push_back(target);
    }
  }

  // If no target has acceptable score, return the best performing one
  if (result.empty() && !available_targets.empty()) {
    result.push_back(available_targets[0]);
  }

  return result;
}

// DpeFactory Implementation
std::unique_ptr<DataPlacementEngine> DpeFactory::CreateDpe(DpeType dpe_type) {
  switch (dpe_type) {
    case DpeType::kRandom:
      return std::make_unique<RandomDpe>();
    case DpeType::kRoundRobin:
      return std::make_unique<RoundRobinDpe>();
    case DpeType::kMaxBW:
      return std::make_unique<MaxBwDpe>();
    default:
      HELOG(kError, "Unknown DPE type, defaulting to Random");
      return std::make_unique<RandomDpe>();
  }
}

std::unique_ptr<DataPlacementEngine> DpeFactory::CreateDpe(const std::string& dpe_str) {
  return CreateDpe(StringToDpeType(dpe_str));
}

} // namespace wrp_cte::core