/*****************************************************************************
 * Copyright [2017] [taurus.ai]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

//
// Created by cjiang on 17/3/24.
//

#include "ft/component/yijinjing/paged/PageEngine.h"

#include <sched.h>
#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <sstream>

#include "ft/component/yijinjing/journal/Page.h"
#include "ft/component/yijinjing/journal/PageSocketStruct.h"
#include "ft/component/yijinjing/journal/PageUtil.h"
#include "ft/component/yijinjing/journal/Timer.h"
#include "ft/component/yijinjing/utils/Hash.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

USING_YJJ_NAMESPACE

const int INTERVAL_IN_MILLISEC = 1000000;

using spdlog::logger;
std::shared_ptr<spdlog::logger> get_console_logger(const std::string& path,
                                                   const std::string& logger_name) {
  auto console_logger = spdlog::get(logger_name);
  if (!console_logger) {
    std::string filename(path);
    filename += logger_name;
    filename += ".log";

    std::vector<spdlog::sink_ptr> sinks;
    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    sinks.push_back(console_sink);

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename);
    file_sink->set_level(spdlog::level::trace);
    sinks.push_back(file_sink);

    console_logger =
        std::make_shared<spdlog::logger>(logger_name, std::begin(sinks), std::end(sinks));

    spdlog::register_logger(console_logger);

    console_logger->set_level(spdlog::level::trace);
    console_logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(console_logger);
  }
  return console_logger;
}

std::atomic<bool> is_task_running(true);

void signal_callback(int signum) {
  if (is_task_running.load(std::memory_order_relaxed)) {
    spdlog::info("PageEngine Caught signal: {}, stopping...", signum);
    is_task_running.store(false);
  } else {
    spdlog::info("PageEngine Caught signal: {}, exit.", signum);
    exit(signum);
  }
}

bool cpu_set_affinity(int cpu_id) {
  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(cpu_id, &mask);
  return 0 == sched_setaffinity(0, sizeof(mask), &mask);
}

void PageEngine::acquire_mutex() { paged_mtx.lock(); }

void PageEngine::release_mutex() { paged_mtx.unlock(); }

PageEngine::PageEngine(const string& commFileName, const std::string& temp_page_file,
                       const std::string& logger_path)
    : commBuffer(nullptr),
      commFile(commFileName),
      maxIdx(0),
      taskFreq(INTERVAL_IN_MILLISEC),
      commFreq(500),
      comm_running(false),
      task_temppage(new PstTempPage(this, temp_page_file)) {
  // setup logger
  logger = get_console_logger(logger_path, "PageEngine");  /// KfLog::getLogger("PageEngine");

  // setup signal related static issue
  //   static_logger = logger;

  for (int s = 1; s < 32; ++s) signal(s, signal_callback);

  /**
      signal(SIGINT, signal_callback);
      signal(SIGTERM, signal_callback);
      signal(SIGABRT, signal_callback);
      //  std::signal(SIGILL, handler);
      //  std::signal(SIGSEGV, handler);
  #if BOOST_OS_WINDOWS
      signal(SIGBREAK, signal_callback);
  #else
      signal(SIGILL, signal_callback);
  #endif
  */
  // setup basic tasks
  tasks.clear();
  add_task(PstBasePtr(new PstPidCheck(this)));  // pid check is a necessary task.
  add_task(task_temppage);                      // temppage is a necessary task

  getNanoTime();  /// for initialize NanoTimer instance;
}

PageEngine::~PageEngine() {}

void PageEngine::start(int cpu_id) {
  spdlog::info("reset socket: {}", PAGED_SOCKET_FILE);
  ///    KF_LOG_INFO(logger, "reset socket: " << PAGED_SOCKET_FILE);
  remove(PAGED_SOCKET_FILE);
  // step 0: init commBuffer
  spdlog::info("loading page buffer: {}", commFile);
  //    KF_LOG_INFO(logger, "loading page buffer: " << commFile);
  commBuffer = PageUtil::LoadPageBuffer(commFile, COMM_SIZE, true, true);
  memset(commBuffer, 0, COMM_SIZE);
  // step 1: start commBuffer checking thread
  comm_running = false;
  commThread = ThreadPtr(new std::thread(std::bind(&PageEngine::start_comm, this, cpu_id)));
  // step 2: start socket listening
  socketThread = ThreadPtr(new std::thread(std::bind(&PageEngine::start_socket, this)));
  // make sure buffer / socket are running
  while (!(PageSocketHandler::getInstance()->is_running() && comm_running)) {
    usleep(INTERVAL_IN_MILLISEC / 10);
  }

  if (taskFreq <= 0) throw std::runtime_error("unaccepted task time interval");

  start_task();
}

void PageEngine::set_task_freq(double secondFreq) {
  taskFreq = (int)(secondFreq * MICROSECONDS_PER_SECOND);
  spdlog::info("task frequency updated to {} microsecond", taskFreq);
}

void PageEngine::set_comm_freq(double secondFreq) {
  commFreq = (int)(secondFreq * MICROSECONDS_PER_SECOND);
  spdlog::info("comm frequency updated to {} microsecond", commFreq);
}

void PageEngine::stop() { is_task_running.store(false); }

void PageEngine::stop_all() {
  /* stop comm thread */
  comm_running = false;
  if (commThread.get() != nullptr) {
    commThread->join();
    commThread.reset();
  }
  spdlog::info("(stopComm) done");

  /* stop socket io thread */
  PageSocketHandler::getInstance()->stop();
  if (socketThread.get() != nullptr) {
    socketThread->join();
    socketThread.reset();
  }
  spdlog::info("(stopSocket) done");

  /* finish up */
  spdlog::info("(stop) done");
}

void PageEngine::start_task() {
  spdlog::info("(startTasks) (microseconds) {}", taskFreq);
  while (is_task_running.load(std::memory_order_relaxed)) {
    acquire_mutex();
    for (auto item : tasks) {
      item.second->go();
    }
    release_mutex();
    usleep(taskFreq);
  }
  spdlog::info("(stopTask) done");
  stop_all();
}

bool PageEngine::add_task(const PstBasePtr& task) {
  acquire_mutex();
  string name = task->getName();
  bool exits = (tasks.find(name) != tasks.end());
  tasks[name] = task;
  spdlog::info("(addTask) (name) {} (exits) {}", name, (int)exits);
  release_mutex();
  return !exits;
}

bool PageEngine::remove_task(const PstBasePtr& task) {
  const string name = task->getName();
  return remove_task_by_name(name);
}

bool PageEngine::remove_task_by_name(const string& taskName) {
  bool flag = false;
  acquire_mutex();
  auto task_iter = tasks.find(taskName);
  if (task_iter == tasks.end()) {
    tasks.erase(task_iter);
    flag = true;
  }
  release_mutex();
  if (flag) {
    spdlog::info("(rmTask) (name) {}", taskName);
  }
  return flag;
}

void PageEngine::start_socket() { PageSocketHandler::getInstance()->run(this); }

int PageEngine::reg_journal(const string& clientName) {
  size_t idx = 0;
  for (; idx < MAX_COMM_USER_NUMBER; idx++)
    if (GET_COMM_MSG(commBuffer, idx)->status == PAGED_COMM_RAW) break;

  if (idx >= MAX_COMM_USER_NUMBER) {
    spdlog::info("cannot allocate idx in commFile");
    return -1;
  }
  if (idx > maxIdx) maxIdx = idx;

  PageCommMsg* msg = GET_COMM_MSG(commBuffer, idx);
  msg->status = PAGED_COMM_OCCUPIED;
  msg->last_page_num = 0;
  auto it = clientJournals.find(clientName);
  if (it == clientJournals.end()) {
    spdlog::info("cannot find the client in reg_journal");
    return -1;
  }
  it->second.user_index_vec.push_back(idx);
  spdlog::info("[RegJournal] (client) {} (idx) {}", clientName, idx);
  return idx;
}

bool PageEngine::reg_client(string& _commFile, int& fileSize, int& hashCode,
                            const string& clientName, int pid, bool isWriter) {
  spdlog::info("[RegClient] (name) {} (pid) {} (writer?) {}", clientName, pid, isWriter);
  if (clientJournals.find(clientName) != clientJournals.end()) {
    spdlog::info("Client already registered (name)", clientName);
    return false;
  }

  map<int, vector<string> >::iterator it = pidClient.find(pid);
  if (it == pidClient.end())
    pidClient[pid] = {clientName};
  else
    pidClient[pid].push_back(clientName);

  std::stringstream ss;
  ss << clientName << getNanoTime() << pid;

  hashCode = MurmurHash2(ss.str().c_str(), ss.str().length(), HASH_SEED);

  PageClientInfo& clientInfo = clientJournals[clientName];
  clientInfo.user_index_vec.clear();
  clientInfo.reg_nano = getNanoTime();
  clientInfo.is_writing = isWriter;
  clientInfo.is_strategy = false;
  clientInfo.rid_start = -1;
  clientInfo.rid_end = -1;
  clientInfo.pid = pid;
  clientInfo.hash_code = hashCode;
  _commFile = this->commFile;
  fileSize = COMM_SIZE;
  return true;
}

void PageEngine::release_page(const PageCommMsg& msg) {
  spdlog::info("[RmPage] (folder) {} (jname) {} (pNum) {} (lpNum) {}", msg.folder, msg.name,
               msg.page_num, msg.last_page_num);

  map<PageCommMsg, int>::iterator count_it;
  if (msg.is_writer) {
    count_it = fileWriterCounts.find(msg);
    if (count_it == fileWriterCounts.end()) {
      spdlog::info("cannot find key at fileWriterCounts in exit_client");
      return;
    }
  } else {
    count_it = fileReaderCounts.find(msg);
    if (count_it == fileReaderCounts.end()) {
      spdlog::info("cannot find key at fileReaderCounts in exit_client");
      return;
    }
  }
  count_it->second--;
  if (count_it->second == 0) {
    bool otherSideEmpty = false;
    if (msg.is_writer) {
      fileWriterCounts.erase(count_it);
      otherSideEmpty = fileReaderCounts.find(msg) == fileReaderCounts.end();
    } else {
      fileReaderCounts.erase(count_it);
      otherSideEmpty = fileWriterCounts.find(msg) == fileWriterCounts.end();
    }
    if (otherSideEmpty) {
      string path = PageUtil::GenPageFullPath(msg.folder, msg.name, msg.page_num);
      auto file_it = fileAddrs.find(path);
      if (file_it != fileAddrs.end()) {
        void* addr = file_it->second;
        spdlog::info("[AddrRm] (path) {} (addr) {}", path, addr);
        PageUtil::ReleasePageBuffer(addr, JOURNAL_PAGE_SIZE, true);
        fileAddrs.erase(file_it);
      }
    }
  }
}

byte PageEngine::initiate_page(const PageCommMsg& msg) {
  spdlog::info("[InPage] (folder) {} (jname) {} (pNum) {} (lpNum) {}", msg.folder, msg.name,
               msg.page_num, msg.last_page_num);

  string path = PageUtil::GenPageFullPath(msg.folder, msg.name, msg.page_num);
  const string temp_full_path = task_temppage->getPath();
  if (fileAddrs.find(path) == fileAddrs.end()) {
    void* buffer = nullptr;
    if (!PageUtil::FileExists(path)) {  // this file is not exist....
      if (!msg.is_writer)
        return PAGED_COMM_NON_EXIST;
      else {
        auto tempPageIter = fileAddrs.find(temp_full_path);
        if (tempPageIter != fileAddrs.end()) {
          int ret = rename(temp_full_path.c_str(), path.c_str());
          if (ret < 0) {
            spdlog::info("[InPage] ERROR: Cannot rename from {} to {}", temp_full_path, path);
            return PAGED_COMM_CANNOT_RENAME_FROM_TEMP;
          } else {
            spdlog::info("[InPage] TEMP_POOL: {} to {}", temp_full_path, path);
            buffer = tempPageIter->second;
            fileAddrs.erase(tempPageIter);
          }
        }
        if (buffer == nullptr)
          buffer = PageUtil::LoadPageBuffer(path, JOURNAL_PAGE_SIZE, true, true);
      }
    } else {  // exist file but not loaded, map and lock immediately.
      buffer = PageUtil::LoadPageBuffer(path, JOURNAL_PAGE_SIZE, false, true);
    }

    spdlog::info("[AddrAdd] (path) {} (addr) {}", path, buffer);
    fileAddrs[path] = buffer;
  }

  if (msg.is_writer) {
    auto count_it = fileWriterCounts.find(msg);
    if (count_it == fileWriterCounts.end())
      fileWriterCounts[msg] = 1;
    else
      return PAGED_COMM_MORE_THAN_ONE_WRITE;
  } else {
    auto count_it = fileReaderCounts.find(msg);
    if (count_it == fileReaderCounts.end())
      fileReaderCounts[msg] = 1;
    else
      count_it->second++;
  }
  return PAGED_COMM_ALLOCATED;
}

void PageEngine::exit_client(const string& clientName, int hashCode, bool needHashCheck) {
  map<string, PageClientInfo>::iterator it = clientJournals.find(clientName);
  if (it == clientJournals.end()) return;
  PageClientInfo& info = it->second;
  if (needHashCheck && hashCode != info.hash_code) {
    spdlog::info("[RmClient] HASH FAILED.. (name) {} (serverHash) {} (clientHash) {}", clientName,
                 info.hash_code, hashCode);
    return;
  }

  for (auto idx : info.user_index_vec) {
    PageCommMsg* msg = GET_COMM_MSG(commBuffer, idx);
    if (msg->status == PAGED_COMM_ALLOCATED) release_page(*msg);
    msg->status = PAGED_COMM_RAW;
  }
  spdlog::info("[RmClient] (name) {} (start) {} (end) {}", clientName, info.reg_nano,
               getNanoTime());
  vector<string>& clients = pidClient[info.pid];
  clients.erase(remove(clients.begin(), clients.end(), clientName), clients.end());
  if (clients.size() == 0) pidClient.erase(info.pid);
  clientJournals.erase(it);
}

void PageEngine::start_comm(int cpu_id) {
  if (cpu_id > 0) {
    if (cpu_set_affinity(cpu_id)) {
      spdlog::info("set cpu_id {} successfully.", cpu_id);
    } else {
      spdlog::info("set cpu_id {} failed.", cpu_id);
    }
  }

  const int comm_freq = commFreq;
  spdlog::info("(startTasks) (microseconds) {}", comm_freq);
  comm_running = true;
  for (size_t idx = 0; comm_running; idx = (idx + 1) % (maxIdx + 1)) {
    PageCommMsg* msg = GET_COMM_MSG(commBuffer, idx);
    if (msg->status == PAGED_COMM_REQUESTING) {
      spdlog::info("[Demand] (idx) {}", idx);

      acquire_mutex();
      if (msg->last_page_num > 0 && msg->last_page_num != msg->page_num) {
        short curPage = msg->page_num;
        msg->page_num = msg->last_page_num;
        release_page(*msg);
        msg->page_num = curPage;
      }
      msg->status = initiate_page(*msg);
      msg->last_page_num = msg->page_num;
      release_mutex();
    }
    usleep(comm_freq);
  }
}
