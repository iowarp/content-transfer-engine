#include <chimaera/core/core_dpe.h>
#include <algorithm>
#include <iostream>
#include <chrono>

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
    std::cerr << "Unknown DPE type: " << dpe_str << ", defaulting to random" << std::endl;
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

std::string RandomDpe::SelectTarget(const std::vector<TargetInfo>& targets, 
                                   float blob_score, 
                                   chi::u64 data_size) {
  if (targets.empty()) {
    return "";
  }

  // Create list of targets with sufficient space
  std::vector<size_t> available_targets;
  for (size_t i = 0; i < targets.size(); ++i) {
    if (targets[i].remaining_space_ >= data_size) {
      available_targets.push_back(i);
    }
  }

  if (available_targets.empty()) {
    return "";  // No targets have space
  }

  // Randomly select from available targets
  std::uniform_int_distribution<size_t> dist(0, available_targets.size() - 1);
  size_t selected_idx = available_targets[dist(rng_)];
  
  return targets[selected_idx].target_name_;
}

// RoundRobinDpe Implementation  
RoundRobinDpe::RoundRobinDpe() {
}

std::string RoundRobinDpe::SelectTarget(const std::vector<TargetInfo>& targets, 
                                       float blob_score, 
                                       chi::u64 data_size) {
  if (targets.empty()) {
    return "";
  }

  chi::u32 counter = round_robin_counter_.fetch_add(1);
  size_t start_idx = counter % targets.size();
  
  // Try each target starting from the round-robin position
  for (size_t i = 0; i < targets.size(); ++i) {
    size_t target_idx = (start_idx + i) % targets.size();
    if (targets[target_idx].remaining_space_ >= data_size) {
      return targets[target_idx].target_name_;
    }
  }

  return "";  // No targets have space
}

// MaxBwDpe Implementation
MaxBwDpe::MaxBwDpe() {
}

std::string MaxBwDpe::SelectTarget(const std::vector<TargetInfo>& targets, 
                                  float blob_score, 
                                  chi::u64 data_size) {
  if (targets.empty()) {
    return "";
  }

  // Create a copy of targets to sort
  std::vector<std::pair<TargetInfo, size_t>> target_pairs;
  for (size_t i = 0; i < targets.size(); ++i) {
    if (targets[i].remaining_space_ >= data_size) {
      target_pairs.emplace_back(targets[i], i);
    }
  }

  if (target_pairs.empty()) {
    return "";  // No targets have space
  }

  // Sort by bandwidth (for I/O >= 32KB) or latency (for I/O < 32KB)
  if (data_size >= kLatencyThreshold) {
    // Sort by write bandwidth (descending)
    std::sort(target_pairs.begin(), target_pairs.end(),
              [](const auto& a, const auto& b) {
                return a.first.perf_metrics_.write_bandwidth_mbps_ > b.first.perf_metrics_.write_bandwidth_mbps_;
              });
  } else {
    // Sort by latency (ascending - lower is better)
    std::sort(target_pairs.begin(), target_pairs.end(),
              [](const auto& a, const auto& b) {
                return (a.first.perf_metrics_.read_latency_us_ + a.first.perf_metrics_.write_latency_us_) / 2.0 < 
                       (b.first.perf_metrics_.read_latency_us_ + b.first.perf_metrics_.write_latency_us_) / 2.0;
              });
  }

  // Find first target with score lower than blob score
  for (const auto& target_pair : target_pairs) {
    if (target_pair.first.target_score_ <= blob_score) {
      return target_pair.first.target_name_;
    }
  }

  // If no target has lower score, return the best performing one
  return target_pairs[0].first.target_name_;
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
      std::cerr << "Unknown DPE type, defaulting to Random" << std::endl;
      return std::make_unique<RandomDpe>();
  }
}

std::unique_ptr<DataPlacementEngine> DpeFactory::CreateDpe(const std::string& dpe_str) {
  return CreateDpe(StringToDpeType(dpe_str));
}

} // namespace wrp_cte::core