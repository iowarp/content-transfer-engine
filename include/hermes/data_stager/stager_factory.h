
#ifndef HERMES_TASKS_DATA_STAGER_SRC_STAGER_FACTORY_H_
#define HERMES_TASKS_DATA_STAGER_SRC_STAGER_FACTORY_H_

#include "abstract_stager.h"
#include "binary_stager.h"

#ifdef HERMES_ENABLE_NVIDIA_GDS_ADAPTER
#include "nvidia_gds_stager.h"
#endif

namespace hermes {

class StagerFactory {
 public:
  static std::unique_ptr<AbstractStager> Get(const std::string &path,
                                             const std::string &params) {
    std::string protocol;
    hipc::LocalDeserialize srl(params);
    srl >> protocol;

    std::unique_ptr<AbstractStager> stager;
    if (protocol == "file" || protocol == "") {
      stager = std::make_unique<BinaryFileStager>();
    } else if (protocol == "parquet") {
    } else if (protocol == "hdf5") {
    }
#ifdef HERMES_ENABLE_NVIDIA_GDS_ADAPTER
    else if (protocol == "nvidia_gds") {
      stager = std::make_unique<NvidiaGdsStager>();
    }
#endif
    else {
      throw std::runtime_error("Unknown stager type");
    }
    stager->path_ = path;
    stager->params_ = params;
    return stager;
  }
};

}  // namespace hermes

#endif  // HERMES_TASKS_DATA_STAGER_SRC_STAGER_FACTORY_H_
