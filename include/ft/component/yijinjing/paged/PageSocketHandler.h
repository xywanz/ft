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

/**
 * Socket Handler for page engine.
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   March, 2017
 */

#ifndef YIJINJING_PAGESOCKETHANDLER_H
#define YIJINJING_PAGESOCKETHANDLER_H

#include <memory>

#include "ft/component/yijinjing/utils/YJJ_DECLARE.h"
#include "ft/component/yijinjing/journal/PageSocketStruct.h"
#include "spdlog/spdlog.h"

YJJ_NAMESPACE_START

/** utilities for socket usage */
class IPageSocketUtil {
 public:
  /** return logger */
  virtual std::shared_ptr<spdlog::logger> get_logger() const = 0;
  /** return journal index in comm file */
  virtual int reg_journal(const string& clientName) = 0;
  /** return true if this client exists, and fill in commfile, size of commfile, hash code of this
   * client */
  virtual bool reg_client(string& commFile, int& fileSize, int& hashCode, const string& clientName,
                          int pid, bool isWriter) = 0;
  /** exit client */
  virtual void exit_client(const string& clientName, int hashCode, bool needHashCheck) = 0;
  /** acquire mutex */
  virtual void acquire_mutex() = 0;
  /** release the mutex */
  virtual void release_mutex() = 0;
};

/** socket handler for page engine */
class PageSocketHandler : public std::enable_shared_from_this<PageSocketHandler> {
 private:
  /** flag for io running */
  bool io_running;
  /** logger, from paged */
  std::shared_ptr<spdlog::logger> logger;
  /** util as page engine */
  IPageSocketUtil* util;
  /** singleton */
  static std::shared_ptr<PageSocketHandler> m_ptr;

 private:
  /** callback when accept new msg*/
  void handle_accept();
  /** minor unit for msg processing */
  void process_msg();
  /** private constructor for singleton */
  PageSocketHandler();

 public:
  /** start run with page engine */
  void run(IPageSocketUtil* _util);
  /** stop io service and collect resource*/
  void stop();
  /** return true if socket io is running */
  bool is_running();
  /** singleton */
  static PageSocketHandler* getInstance();
};

YJJ_NAMESPACE_END

#endif  // YIJINJING_PAGESOCKETHANDLER_H
