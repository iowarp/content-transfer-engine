#include <iostream>
#include <random>
#include <map>
#include <numeric>

#include "hermes.h"
#include "data_placement_engine.h"
#include "test_utils.h"

using namespace hermes;  // NOLINT(*)

void RoundRobinPlaceBlob(std::vector<size_t> &blob_sizes,
                         std::vector<PlacementSchema> &schemas,
                         testing::TargetViewState &node_state) {
  std::vector<PlacementSchema> schemas_tmp;
  std::cout << "\nRoundRobinPlacement to place blob of size "
            << blob_sizes[0] << " to targets\n" << std::flush;

  std::vector<TargetID> targets =
    testing::GetDefaultTargets(node_state.num_devices);
  Status result = RoundRobinPlacement(blob_sizes, node_state.bytes_available,
                                      schemas_tmp, targets);
  if (result) {
    std::cout << "\nRoundRobinPlacement failed\n" << std::flush;
    exit(1);
  }

  for (auto it = schemas_tmp.begin(); it != schemas_tmp.end(); ++it) {
    PlacementSchema schema = AggregateBlobSchema((*it));
    Assert(schemas.size() <= static_cast<size_t>(node_state.num_devices));
    schemas.push_back(schema);
  }

  u64 placed_size {0};
  for (auto schema : schemas) {
    placed_size += UpdateDeviceState(schema, node_state);
  }

  std::cout << "\nUpdate Device State:\n";
  testing::PrintNodeState(node_state);
  u64 total_sizes = std::accumulate(blob_sizes.begin(), blob_sizes.end(), 0);
  Assert(placed_size == total_sizes);
}

int main() {
  testing::TargetViewState node_state{testing::InitDeviceState()};

  Assert(node_state.num_devices == 4);
  std::cout << "Device Initial State:\n";
  testing::PrintNodeState(node_state);

  std::vector<size_t> blob_sizes1(1, MEGABYTES(10));
  std::vector<PlacementSchema> schemas1;
  RoundRobinPlaceBlob(blob_sizes1, schemas1, node_state);
  Assert(schemas1.size() == blob_sizes1.size());

  std::vector<size_t> blob_sizes2(1, MEGABYTES(1));
  std::vector<PlacementSchema> schemas2;
  RoundRobinPlaceBlob(blob_sizes2, schemas2, node_state);
  Assert(schemas2.size() == blob_sizes2.size());

  return 0;
}