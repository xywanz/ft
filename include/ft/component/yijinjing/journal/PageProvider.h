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
 * Page Provider.
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   March, 2017
 * implements IPageProvider, diverge to different usage
 */

#ifndef YIJINJING_PAGEPROVIDER_H
#define YIJINJING_PAGEPROVIDER_H

#include "ft/component/yijinjing/utils/YJJ_DECLARE.h"

YJJ_NAMESPACE_START

/**
 * PageProvider,
 * provide page via memory service, socket & comm
 */

FORWARD_DECLARE_PTR(Page);

class PageProvider {
 protected:
  /** true if provider is used by a JournalWriter */
  const bool is_writer;

  const string client_name;
  void* comm_buffer;
  int hash_code;

 public:
  /** return true if this is for writing */
  bool isWriter() const { return is_writer; };

 protected:
  /** register to service as a client */
  void register_client();

 public:
  /** default constructor with client name and writing flag */
  PageProvider(const string& clientName, bool isWriting);

  /** register journal when added into JournalHandler */
  int register_journal(const string& dir, const string& jname);
  /** exit client after JournalHandler is released */
  void exit_client();

  /** return wrapped Page via directory / journal short name / serviceIdx assigned / page number */
  PagePtr getPage(const string& dir, const string& jname, int serviceIdx, short pageNum);
  /** release page after using */
  void releasePage(void* buffer, int size, int serviceIdx);
};

DECLARE_PTR(PageProvider);

YJJ_NAMESPACE_END

#endif  // YIJINJING_PAGEPROVIDER_H
