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

#include "random.h"

#include <utility>
#include <random>
#include <map>

namespace hermes {

Status Random::Placement(const std::vector<size_t> &blob_sizes,
                         std::vector<TargetInfo> &targets,
                         api::Context &ctx,
                         std::vector<PlacementSchema> &output) {
  throw std::logic_error("Not currently implemented");
}

}  // namespace hermes
