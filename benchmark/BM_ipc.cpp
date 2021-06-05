// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>

#include "ft/base/market_data.h"
#include "ft/base/trade_msg.h"
#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"

static void BM_yijinjing_ipc(benchmark::State& state) {
  auto md_writer = yijinjing::JournalWriter::create(".", "BM_yijinjing_ipc_md", "md_writer");
  auto md_reader = yijinjing::JournalReader::create(".", "BM_yijinjing_ipc_md",
                                                    yijinjing::TIME_TO_LAST, "md_reader");

  auto td_writer = yijinjing::JournalWriter::create(".", "BM_yijinjing_ipc_td", "td_writer");
  auto td_reader = yijinjing::JournalReader::create(".", "BM_yijinjing_ipc_td",
                                                    yijinjing::TIME_TO_LAST, "td_reader");

  std::atomic<bool> running = true;

  std::thread st_thread([&] {
    ft::TraderCommand cmd;

    while (running) {
      if (!md_reader->getNextFrame()) {
        continue;
      }
      td_writer->write_data(cmd, 0, 0);
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ft::TickData tick;
  for (auto _ : state) {
    md_writer->write_data(tick, 0, 0);
    while (!td_reader->getNextFrame()) {
      continue;
    }
  }

  running = false;
  st_thread.join();

  int res = system("rm -f yjj.BM_yijinjing_ipc*");
  (void)res;
}
BENCHMARK(BM_yijinjing_ipc);
BENCHMARK_MAIN();
