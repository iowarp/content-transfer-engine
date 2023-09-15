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

#include <iostream>
#include <hermes_shm/util/timer.h>
#include <mpi.h>
#include "hermes/hermes.h"
#include "hermes/bucket.h"
#include "labstor/work_orchestrator/affinity.h"

namespace hapi = hermes;
using Timer = hshm::HighResMonotonicTimer;

/** Gather times per-process */
void GatherTimes(std::string test_name, size_t io_size, Timer &t) {
  MPI_Barrier(MPI_COMM_WORLD);
  int rank, nprocs;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  double time = t.GetSec(), max;
  MPI_Reduce(&time, &max,
             1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    double mbps = io_size / (max * 1000000);
    HIPRINT("{}: Time: {} sec, MBps (or MOps): {}, Count: {}, Nprocs: {}\n",
            test_name, max, mbps, io_size, nprocs);
  }
}

/** Each process PUTS into the same bucket, but with different blob names */
void PutTest(int nprocs, int rank,
             int repeat, size_t blobs_per_rank, size_t blob_size) {
  Timer t;
  hermes::Context ctx;
  hermes::Bucket bkt("hello", ctx);
  hermes::Blob blob(blob_size);
  t.Resume();
  for (int j = 0; j < repeat; ++j) {
    for (size_t i = 0; i < blobs_per_rank; ++i) {
      size_t blob_name_int = rank * blobs_per_rank + i;
      std::string name = std::to_string(blob_name_int);
      bkt.Put(name, blob, ctx);
    }
  }
  t.Pause();
  GatherTimes("Put", nprocs * blobs_per_rank * blob_size * repeat, t);
}

/**
 * Each process GETS from the same bucket, but with different blob names
 * MUST run PutTest first.
 * */
void GetTest(int nprocs, int rank,
             int repeat, size_t blobs_per_rank, size_t blob_size) {
  Timer t;
  hermes::Context ctx;
  hermes::Bucket bkt("hello", ctx);
  t.Resume();
  for (int j = 0; j < repeat; ++j) {
    for (size_t i = 0; i < blobs_per_rank; ++i) {
      size_t blob_name_int = rank * blobs_per_rank + i;
      std::string name = std::to_string(blob_name_int);
      hermes::Blob ret;
      hermes::BlobId blob_id = bkt.GetBlobId(name);
      bkt.Get(blob_id, ret, ctx);
    }
  }
  t.Pause();
  GatherTimes("Get", nprocs * blobs_per_rank * blob_size * repeat, t);
}

/** Each process PUTs then GETs */
void PutGetTest(int nprocs, int rank, int repeat,
                size_t blobs_per_rank, size_t blob_size) {
  PutTest(nprocs, rank, repeat, blobs_per_rank, blob_size);
  MPI_Barrier(MPI_COMM_WORLD);
  GetTest(nprocs, rank, repeat, blobs_per_rank, blob_size);
}

/** Each process creates a set of buckets */
void CreateBucketTest(int nprocs, int rank,
                      size_t bkts_per_rank) {
  Timer t;
  t.Resume();
  hapi::Context ctx;
  std::unordered_map<std::string, std::string> mdm_;
  for (size_t i = 0; i < bkts_per_rank; ++i) {
    int bkt_name_int = rank * bkts_per_rank + i;
    std::string bkt_name = std::to_string(bkt_name_int);
    hermes::Bucket bkt(bkt_name, ctx);
  }
  t.Pause();
  GatherTimes("CreateBucket", bkts_per_rank * nprocs, t);
}

/** Each process gets existing buckets */
void GetBucketTest(int nprocs, int rank,
                   size_t bkts_per_rank) {
  // Initially create the buckets
  hapi::Context ctx;
  for (size_t i = 0; i < bkts_per_rank; ++i) {
    int bkt_name = rank * bkts_per_rank + i;
    hermes::Bucket bkt(std::to_string(bkt_name), ctx);
  }

  // Get existing buckets
  Timer t;
  t.Resume();
  for (size_t i = 0; i < bkts_per_rank; ++i) {
    int bkt_name = rank * bkts_per_rank + i;
    hapi::Bucket bkt(std::to_string(bkt_name), ctx);
  }
  t.Pause();
  GatherTimes("CreateBucket", bkts_per_rank * nprocs, t);
}

/** Each process deletes a number of buckets */
void DeleteBucketTest(int nprocs, int rank,
                      size_t bkt_per_rank,
                      size_t blobs_per_bucket) {
  Timer t;
  hapi::Context ctx;

  // Create the buckets
  for (size_t i = 0; i < bkt_per_rank; ++i) {
    hapi::Bucket bkt(hshm::Formatter::format("DeleteBucket{}", rank), ctx);
    hapi::Blob blob;
    for (size_t j = 0; j < blobs_per_bucket; ++j) {
      std::string name = std::to_string(j);
      bkt.Put(name, blob,  ctx);
    }
  }

  // Delete the buckets
  t.Resume();
  for (size_t i = 0; i < bkt_per_rank; ++i) {
    hapi::Bucket bkt(hshm::Formatter::format("DeleteBucket{}", rank), ctx);
    bkt.Destroy();
  }
  t.Pause();
  GatherTimes("DeleteBucket", nprocs * bkt_per_rank * blobs_per_bucket, t);
}

/** Each process deletes blobs from a single bucket */
void DeleteBlobOneBucket(int nprocs, int rank,
                         size_t blobs_per_rank) {
}


#define REQUIRE_ARGC_GE(N) \
  if (argc < (N)) { \
    HIPRINT("Requires fewer than {} params\n", N); \
    help(); \
  }

#define REQUIRE_ARGC(N) \
  if (argc != (N)) { \
    HIPRINT("Requires exactly {} params\n", N); \
    help(); \
  }

void help() {
  printf("USAGE: ./api_bench [mode] ...\n");
  printf("USAGE: ./api_bench put [blob_size (K/M/G)] [blobs_per_rank]\n");
  printf("USAGE: ./api_bench putget [blob_size (K/M/G)] [blobs_per_rank]\n");
  printf("USAGE: ./api_bench create_bkt [bkts_per_rank]\n");
  printf("USAGE: ./api_bench get_bkt [bkts_per_rank]\n");
  printf("USAGE: ./api_bench create_blob_1bkt [blobs_per_rank]\n");
  printf("USAGE: ./api_bench create_blob_Nbkt [blobs_per_rank]\n");
  printf("USAGE: ./api_bench del_bkt [bkt_per_rank] [blobs_per_bkt]\n");
  printf("USAGE: ./api_bench del_blobs [blobs_per_rank]\n");
  exit(1);
}

int main(int argc, char **argv) {
  int rank, nprocs;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  TRANSPARENT_LABSTOR();
  HERMES->ClientInit();

  // Get mode
  REQUIRE_ARGC_GE(2)
  std::string mode = argv[1];

  // Set CPU affinity
  // TODO(logan): remove
  ProcessAffiner::SetCpuAffinity(getpid(), 8);

  MPI_Barrier(MPI_COMM_WORLD);

  HIPRINT("Beginning {}\n", mode)

  // Run tests
  if (mode == "put") {
    REQUIRE_ARGC(4)
    size_t blob_size = hshm::ConfigParse::ParseSize(argv[2]);
    size_t blobs_per_rank = atoi(argv[3]);
    PutTest(nprocs, rank, 1, blobs_per_rank, blob_size);
  } else if (mode == "putget") {
    REQUIRE_ARGC(4)
    size_t blob_size = hshm::ConfigParse::ParseSize(argv[2]);
    size_t blobs_per_rank = atoi(argv[3]);
    PutGetTest(nprocs, rank, 1, blobs_per_rank, blob_size);
  } else if (mode == "create_bkt") {
    REQUIRE_ARGC(3)
    size_t bkts_per_rank = atoi(argv[2]);
    CreateBucketTest(nprocs, rank, bkts_per_rank);
  } else if (mode == "get_bkt") {
    REQUIRE_ARGC(3)
    size_t bkts_per_rank = atoi(argv[2]);
    GetBucketTest(nprocs, rank, bkts_per_rank);
  } else if (mode == "del_bkt") {
    REQUIRE_ARGC(4)
    size_t bkt_per_rank = atoi(argv[2]);
    size_t blobs_per_bkt = atoi(argv[3]);
    DeleteBucketTest(nprocs, rank, bkt_per_rank, blobs_per_bkt);
  } else if (mode == "del_blobs") {
    REQUIRE_ARGC(4)
    size_t blobs_per_rank = atoi(argv[2]);
    DeleteBlobOneBucket(nprocs, rank, blobs_per_rank);
  }
  MPI_Finalize();
}