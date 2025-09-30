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

#include <cassert>

#include "hermes/hermes.h"

int main() {
  TRANSPARENT_HERMES();
  hermes::Bucket bkt("hello");
  size_t blob_size = KILOBYTES(4);
  hermes::Context ctx;

  std::vector<int> data_put(1024, 0);
  bkt.Put<std::vector<int>>("0", data_put, ctx);

  std::vector<int> data_get(1024, 1);
  bkt.Get<std::vector<int>>("0", data_get, ctx);

  assert(data_put == data_get);
}