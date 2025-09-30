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
#define HERMES_ENABLE_GDS_STAGER
#include <mpi.h>

#define HERMES_ENABLE_NVIDIA_GDS_ADAPTER
#include "hermes/data_stager/stager_factory.h"

HSHM_GPU_KERNEL void test_bucket(hermes::Bucket bkt) {
  for (int i = 0; i < 10; ++i) {
    printf("IN KERNEL!!! %p\n", CHI_CLIENT->data_alloc_);
    hermes::Blob blob(1024);
    printf("I DIDN'T DIE?\n");
    memset(blob.data(), 10, blob.size());
    printf("I STILL DIDN'T DIE?\n");
    bkt.Put(chi::string("a"), blob, hermes::Context());
    printf("Finished put\n");
    hermes::Blob blob2(1024);
    memset(blob2.data(), 1, blob2.size());
    printf("Blob2 data set\n");
    bkt.Get(chi::string("a"), blob2, hermes::Context());
    printf("Got blob2 data\n");
    for (int i = 0; i < blob2.size(); ++i) {
      if (blob2.data()[i] != 10) {
        return;
      }
    }
    printf("SUCESS!!!\n");
  }
}

int main(int argc, char **argv) {
  HERMES_INIT();
  hermes::Bucket bkt(chi::string("bucket"));
  HILOG(kInfo, "Created bucket: {}", bkt.GetId());
  test_bucket<<<1, 1>>>(bkt);
  hshm::GpuApi::Synchronize();
  return 0;
}
