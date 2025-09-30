//
// Created by llogan on 7/1/23.
//

#include <thread>

#include "basic_test.h"
#include "chimaera/api/chimaera_client.h"
#include "chimaera/api/chimaera_runtime.h"
#include "chimaera/work_orchestrator/affinity.h"
#include "chimaera_admin/chimaera_admin_client.h"
#include "hermes/hermes.h"
#include "hermes_shm/util/timer.h"
#include "small_message/small_message.h"

/** Time to process a request */
// TEST_CASE("TestHermesGetBlobIdLatency") {
//   HERMES->ClientInit();
//   hshm::Timer t;
//
//   int pid = getpid();
//   ProcessAffiner::SetCpuAffinity(pid, 8);
//   hermes::Bucket bkt =
//   HERMES_FILESYSTEM_API->Open("/home/lukemartinlogan/hi.txt");
//
//   t.Resume();
//   size_t ops = (1 << 20);
//   hermes::Context ctx;
//   std::string data(ctx.page_size_, 0);
//   for (size_t i = 0; i < ops; ++i) {
//     bkt.GetBlobId(std::to_string(i));
//   }
//   t.Pause();
//
//   HILOG(kInfo, "Latency: {} MOps", ops / t.GetUsec());
// }
//
///** Time to process a request */
// TEST_CASE("TestHermesFsWriteLatency") {
//   HERMES->ClientInit();
//   hshm::Timer t;
//
//   int pid = getpid();
//   ProcessAffiner::SetCpuAffinity(pid, 8);
//   hermes::Bucket bkt =
//   HERMES_FILESYSTEM_API->Open("/home/lukemartinlogan/hi.txt");
//
//   t.Resume();
//   size_t ops = 150;
//   hermes::Context ctx;
//   ctx.page_size_ = 4096;
//   std::string data(ctx.page_size_, 0);
//   for (size_t i = 0; i < ops; ++i) {
//     HERMES_FILESYSTEM_API->Write(bkt, data.data(), i * ctx.page_size_,
//     data.size(), false, rctx);
//   }
//   t.Pause();
//
//   HILOG(kInfo, "Latency: {} MBps", ops * 4096 / t.GetUsec());
// }
//
///** Time to process a request */
// TEST_CASE("TestHermesFsReadLatency") {
//   HERMES->ClientInit();
//   hshm::Timer t;
//
//   int pid = getpid();
//   ProcessAffiner::SetCpuAffinity(pid, 8);
//   hermes::Bucket bkt =
//   HERMES_FILESYSTEM_API->Open("/home/lukemartinlogan/hi.txt");
//
//   size_t ops = 1024;
//   hermes::Context ctx;
//   ctx.page_size_ = 4096;
//   std::string data(ctx.page_size_, 0);
//   for (size_t i = 0; i < ops; ++i) {
//     HERMES_FILESYSTEM_API->Write(bkt, data.data(), i * ctx.page_size_,
//     data.size(), false, rctx);
//   }
//
//   t.Resume();
//   for (size_t i = 0; i < ops; ++i) {
//     HERMES_FILESYSTEM_API->Read(bkt, data.data(), i * ctx.page_size_,
//     data.size(), false, rctx);
//   }
//   t.Pause();
//
//   HILOG(kInfo, "Latency: {} MBps", ops * 4096 / t.GetUsec());
// }