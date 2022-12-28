//
// Created by lukemartinlogan on 12/27/22.
//

#include <iostream>
#include <labstor/util/timer.h>
#include <mpi.h>
#include "hermes.h"
#include "bucket.h"

namespace hapi = hermes::api;
using Timer = labstor::HighResMonotonicTimer;

void GatherTimes(Timer &t) {
  MPI_Barrier(MPI_COMM_WORLD);
  int nprocs, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  double time = t.GetSec(), max;
  MPI_Reduce(&time, &max,
             1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    std::cout << "Time (sec): " << max << std::endl;
  }
}

void PutTest(hapi::Hermes *hermes,
             int rank, int blobs_per_rank, size_t blob_size) {
  Timer t;
  t.Resume();
  auto bkt = hermes->GetBucket("hello");
  hermes::api::Context ctx;
  hermes::BlobId blob_id;
  hermes::Blob blob(nullptr, blob_size);
  for (size_t i = 0; i < blobs_per_rank; ++i) {
    size_t blob_name_int = rank * blobs_per_rank + i;
    std::string name = std::to_string(blob_name_int);
    bkt->Put(name, std::move(blob), blob_id, ctx);
  }
  t.Pause();
  GatherTimes(t);
}

void GetTest(hapi::Hermes *hermes,
             int rank, int blobs_per_rank, size_t blob_size) {
  Timer t;
  t.Resume();
  auto bkt = hermes->GetBucket("hello");
  hermes::api::Context ctx;
  hermes::BlobId blob_id;
  hermes::Blob blob(nullptr, blob_size);
  for (size_t i = 0; i < blobs_per_rank; ++i) {
    size_t blob_name_int = rank * blobs_per_rank + i;
    std::string name = std::to_string(blob_name_int);
    hermes::Blob ret;
    bkt->GetBlobId(name, blob_id, ctx);
    bkt->Get(blob_id, ret, ctx);
  }
  t.Pause();
  GatherTimes(t);
}

int main(int argc, char **argv) {
  int rank;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  auto hermes = hapi::Hermes::Create(hermes::HermesType::kClient);
  int blobs_per_rank = 1024;
  size_t blob_size = KILOBYTES(64);
  PutTest(hermes, rank, blobs_per_rank, blob_size);
  GetTest(hermes, rank, blobs_per_rank, blob_size);
  hermes->Finalize();
  MPI_Finalize();
}