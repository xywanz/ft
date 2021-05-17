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

#include "ft/component/yijinjing/journal/Page.h"
#include "ft/component/yijinjing/journal/PageCommStruct.h"
#include "ft/component/yijinjing/journal/PageProvider.h"
#include "ft/component/yijinjing/journal/PageSocketStruct.h"
#include "ft/component/yijinjing/journal/PageUtil.h"

USING_YJJ_NAMESPACE

void PageProvider::register_client() {}

void PageProvider::exit_client() {}

int PageProvider::register_journal(const string& dir, const string& jname) { return -1; };

PageProvider::PageProvider(const string& clientName, bool isWriting)
    : is_writer(isWriting), client_name(clientName), comm_buffer(nullptr), hash_code(0) {
  //  is_writer = isWriting;
  //  revise_allowed = is_writer || reviseAllowed;
}

PagePtr PageProvider::getPage(const string& dir, const string& jname, int serviceIdx,
                              short pageNum) {
  return Page::load(dir, jname, pageNum, is_writer, false);
}

void PageProvider::releasePage(void* buffer, int size, int serviceIdx) {
  PageUtil::ReleasePageBuffer(buffer, size, false);
}
