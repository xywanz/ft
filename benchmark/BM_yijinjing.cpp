// Copyright [2020-present] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <algorithm>
#include <ctime>
#include <thread>

#include "ft/base/market_data.h"
#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/component/yijinjing/journal/Timer.h"

constexpr uint64_t kLoopTimes = 100000UL;

yijinjing::JournalWriterPtr writer;
yijinjing::JournalReaderPtr reader;

void WriteThread() {
  ft::TickData tick{};
  for (uint64_t i = 0; i < kLoopTimes; ++i) {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    tick.local_timestamp_us = ts.tv_nsec + ts.tv_sec * 1000000000UL;
    writer->write_data(tick, 0, 0);
  }
}

void ReadThread() {
  std::vector<uint64_t> time_cost;
  yijinjing::FramePtr frame;
  for (uint64_t i = 0; i < kLoopTimes; ++i) {
    do {
      frame = reader->getNextFrame();
    } while (!frame);
    auto* tick = reinterpret_cast<ft::TickData*>(frame->getData());
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    time_cost.emplace_back(ts.tv_nsec + ts.tv_sec * 1000000000UL - tick->local_timestamp_us);
  }

  sort(time_cost.begin(), time_cost.end());

  uint64_t sum = 0;
  for (auto v : time_cost) {
    sum += v;
  }
  printf("mean:%lu, min:%lu, 25th:%lu, 50th:%lu 75th:%lu max:%lu\n", sum / kLoopTimes, time_cost[0],
         time_cost[time_cost.size() / 4], time_cost[time_cost.size() / 2],
         time_cost[time_cost.size() * 3 / 4], *time_cost.rbegin());
}

int main() {
  writer = yijinjing::JournalWriter::create(".", "BM_yijinjing", "writer");
  reader =
      yijinjing::JournalReader::create(".", "BM_yijinjing", yijinjing::getNanoTime(), "reader");

  std::thread rd_thread(ReadThread);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::thread wr_thread(WriteThread);

  wr_thread.join();
  rd_thread.join();

  int res = system("rm -f yjj.BM_yijinjing.*");
  (void)res;
}
