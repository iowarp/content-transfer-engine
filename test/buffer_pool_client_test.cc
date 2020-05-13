#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <utility>

#include <mpi.h>

#include <thallium.hpp>
#include <thallium/serialization/stl/vector.hpp>
#include <thallium/serialization/stl/pair.hpp>

#include "common.h"
#include "buffer_pool.h"
#include "buffer_pool_internal.h"
#include "utils.h"

/**
 * @file buffer_pool_client_test.cc
 *
 * This shows how an application core might interact with the BufferPool. A
 * client must open the existing BufferPool shared memory (created by the hermes
 * core intialization), intialize communication, set up files for buffering, and
 * can then call the BufferPool either directly (same node) or via RPC (remote
 * node).
 */

namespace tl = thallium;

using namespace hermes;

static constexpr char kServerName[] = "ofi+sockets://127.0.0.1:8080";
static constexpr char kBaseShemeName[] = "/hermes_buffer_pool_";

struct TimingResult {
  double get_buffers_time;
  double release_buffers_time;
};

TimingResult TestGetBuffersRpc(int iters) {
  TimingResult result = {};

  tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
  tl::remote_procedure get_buffers = myEngine.define("GetBuffers");
  tl::remote_procedure release_buffers =
    myEngine.define("ReleaseBuffers").disable_response();
  tl::endpoint server = myEngine.lookup(kServerName);
  TieredSchema schema{std::make_pair(4096, 0)};

  Timer get_timer;
  Timer release_timer;
  for (int i = 0; i < iters; ++i) {
    get_timer.resumeTime();
    std::vector<BufferID> ret = get_buffers.on(server)(schema);
    get_timer.pauseTime();

    if (ret.size() == 0) {
      break;
    }

    release_timer.resumeTime();
    release_buffers.on(server)(ret);
    release_timer.pauseTime();
  }

  result.get_buffers_time = get_timer.getElapsedTime();
  result.release_buffers_time = release_timer.getElapsedTime();

  return result;
}

TimingResult TestGetBuffers(SharedMemoryContext *context, int iters) {
  TimingResult result = {};
  TieredSchema schema{std::make_pair(4096, 0)};

  Timer get_timer;
  Timer release_timer;
  for (int i = 0; i < iters; ++i) {
    get_timer.resumeTime();
    std::vector<BufferID> ret = GetBuffers(context, schema);
    get_timer.pauseTime();

    if (ret.size() == 0) {
      break;
    }

    release_timer.resumeTime();
    LocalReleaseBuffers(context, ret);
    release_timer.pauseTime();
  }

  result.get_buffers_time = get_timer.getElapsedTime();
  result.release_buffers_time = release_timer.getElapsedTime();

  return result;
}

double TestSplitBuffers(SharedMemoryContext *context, int slab_index,
                        bool use_rpc) {
  Timer timer;
  if (use_rpc) {
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure split_buffers =
      myEngine.define("SplitBuffers").disable_response();
    tl::endpoint server = myEngine.lookup(kServerName);
    timer.resumeTime();
    split_buffers.on(server)(slab_index);
    timer.pauseTime();
  } else {
    timer.resumeTime();
    SplitRamBufferFreeList(context, slab_index);
    timer.pauseTime();
  }
  double result = timer.getElapsedTime();
  // TODO(chogan): Verify results
  return result;
}

double TestMergeBuffers(SharedMemoryContext *context, int slab_index,
                        bool use_rpc) {
  Timer timer;

  if (use_rpc) {
    tl::engine myEngine("tcp", THALLIUM_CLIENT_MODE);
    tl::remote_procedure merge_buffers =
      myEngine.define("MergeBuffers").disable_response();
    tl::endpoint server = myEngine.lookup(kServerName);
    timer.resumeTime();
    merge_buffers.on(server)(slab_index);
    timer.pauseTime();
  } else {
    timer.resumeTime();
    MergeRamBufferFreeList(context, slab_index);
    timer.pauseTime();
  }
  double result = timer.getElapsedTime();
  // TODO(chogan): Verify results
  return result;
}

void TestFileBuffering(SharedMemoryContext *context, int rank) {
  Blob blob = {};
  TierID tier_id = 1;

  const char test_file[] = "bp_viz/bpm_snapshot_0.bmp";
  FILE *f = fopen(test_file, "r");
  if (f) {
    fseek(f, 0, SEEK_END);
    blob.size = ftell(f);
    blob.data =  (u8 *)mmap(0, blob.size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
    fclose(f);
    if (blob.data) {
      TieredSchema schema{std::make_pair(blob.size, tier_id)};
      std::vector<BufferID> buffer_ids(0);

      while (buffer_ids.size() == 0) {
        buffer_ids = GetBuffers(context, schema);
      }

      WriteBlobToBuffers(context, blob, buffer_ids);

      std::vector<u8> data(blob.size);
      Blob result = {};
      result.size = blob.size;
      result.data = data.data();

      BufferIdArray buffer_id_arr = {};
      buffer_id_arr.ids = buffer_ids.data();
      buffer_id_arr.length = buffer_ids.size();
      // TODO(chogan): Pass rpc here
      ReadBlobFromBuffers(context,  NULL, &result, &buffer_id_arr);

      std::stringstream out_filename_stream;
      out_filename_stream << "TestfileBuffering_rank" << std::to_string(rank)
                          << ".bmp";
      std::string out_filename = out_filename_stream.str();
      FILE *out_file = fopen(out_filename.c_str(), "w");
      size_t items_written = fwrite(result.data, result.size, 1, out_file);
      assert(items_written == 1);
      fclose(out_file);

      LocalReleaseBuffers(context, buffer_ids);
    } else {
      perror(0);
    }
  } else {
    perror(0);
  }
  munmap(blob.data, blob.size);
}

void PrintUsage(char *program) {
  fprintf(stderr, "Usage %s [-r]\n", program);
  fprintf(stderr, "  -b\n");
  fprintf(stderr, "     Run GetBuffers test.\n");
  fprintf(stderr, "  -f\n");
  fprintf(stderr, "     Run FileBuffering test.\n");
  fprintf(stderr, "  -i <num>\n");
  fprintf(stderr, "     Use <num> iterations in GetBuffers test.\n");
  fprintf(stderr, "  -m <num>\n");
  fprintf(stderr, "     Run MergeBuffers test on slab <num>.\n");
  fprintf(stderr, "  -r\n");
  fprintf(stderr, "     Run GetBuffers test using RPC.\n");
  fprintf(stderr, "  -s <num>\n");
  fprintf(stderr, "     Run SplitBuffers test on slab <num>.\n");
}

int main(int argc, char **argv) {
  int mpi_threads_provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mpi_threads_provided);
  if (mpi_threads_provided < MPI_THREAD_MULTIPLE) {
    fprintf(stderr, "Didn't receive appropriate MPI threading specification\n");
    return 1;
  }
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int option = -1;
  bool test_get_release = false;
  bool use_rpc = false;
  bool test_split = false;
  bool test_merge = false;
  bool test_file_buffering = false;
  bool kill_server = false;
  int slab_index = 0;
  int iters = 100000;
  int pid = 0;

  while ((option = getopt(argc, argv, "bfkrs:i:m:p:")) != -1) {
    switch (option) {
      case 'b': {
        test_get_release = true;
        break;
      }
      case 'f': {
        test_file_buffering = true;
        test_get_release = false;
        break;
      }
      case 'i': {
        iters = atoi(optarg);
        break;
      }
      case 'k': {
        kill_server = true;
        break;
      }
      case 'm': {
        test_merge = true;
        slab_index = atoi(optarg) - 1;
        test_get_release = false;
        break;
      }
      case 'p': {
        pid = atoi(optarg);
        break;
      }
      case 'r': {
        use_rpc = true;
        break;
      }
      case 's': {
        test_split = true;
        slab_index = atoi(optarg) - 1;
        test_get_release = false;
        break;
      }
      default:
        PrintUsage(argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
  }

  char full_shmem_name[hermes::kMaxBufferPoolShmemNameLength];
  MakeFullShmemName(full_shmem_name, kBaseShemeName);
  // TODO(chogan): Client side comm and rpc initialization
  SharedMemoryContext context = InitHermesClient(NULL, full_shmem_name,
                                                 test_file_buffering);

  if (test_get_release) {
    TimingResult timing = {};

    if (use_rpc) {
      timing = TestGetBuffersRpc(iters);
    } else {
      timing = TestGetBuffers(&context, iters);
    }

    int total_iters = iters * world_size;
    double total_get_seconds = 0;
    double total_release_seconds = 0;
    MPI_Reduce(&timing.get_buffers_time, &total_get_seconds, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&timing.release_buffers_time, &total_release_seconds, 1,
               MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (world_rank == 0) {
      double avg_get_seconds = total_get_seconds / world_size;
      double avg_release_seconds = total_release_seconds / world_size;
      double gets_per_second = total_iters / avg_get_seconds;
      double releases_per_second = total_iters / avg_release_seconds;
      printf("%f %f ", gets_per_second, releases_per_second);
    }
  }

  if (test_split) {
    assert(world_size == 1);
    double seconds_for_split = TestSplitBuffers(&context, slab_index, use_rpc);
    printf("%f\n", seconds_for_split);
  }
  if (test_merge) {
    assert(world_size == 1);
    double seconds_for_merge = TestMergeBuffers(&context, slab_index, use_rpc);
    printf("%f\n", seconds_for_merge);
  }
  if (test_file_buffering) {
    TestFileBuffering(&context, world_rank);
  }

  ReleaseSharedMemoryContext(&context);
  MPI_Barrier(MPI_COMM_WORLD);

  if (world_rank == 0 && kill_server) {
    // NOTE(chogan): Shut down the RPC server
    std::string server_name;
    if (pid) {
      std::string shm_id = "0";
      std::string shm_pid = std::to_string(pid);
      server_name = "na+sm://" + shm_pid + "/" + shm_id;
    } else {
      server_name = kServerName;
    }
    tl::engine myEngine(server_name, THALLIUM_CLIENT_MODE);
    tl::remote_procedure finalize =
      myEngine.define("Finalize").disable_response();
    tl::endpoint server = myEngine.lookup(server_name);
    finalize.on(server)();
  }

  MPI_Finalize();

  return 0;
}
