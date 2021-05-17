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
 * JournalWriter
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   March, 2017
 * enable user to write in journal.
 * one journal writer can only write one journal,
 * and meanwhile this journal cannot be linked by other writer
 */

#ifndef YIJINJING_JOURNALWRITER_H
#define YIJINJING_JOURNALWRITER_H

#include "ft/component/yijinjing/journal/JournalHandler.h"
//#include "FrameHeader.h"
#include "ft/component/yijinjing/journal/Frame.hpp"

YJJ_NAMESPACE_START

FORWARD_DECLARE_PTR(JournalWriter)
/**
 * Journal Writer
 */
class JournalWriter : public JournalHandler {
 protected:
  /** the journal will write in */
  JournalPtr journal;
  /** private constructor */
  JournalWriter(PageProviderPtr& ptr) : JournalHandler(ptr) {}

 public:
  /** init journal */
  void init(const string& dir, const string& jname);
  /** get current page number */
  short getPageNum() const;
  /* seek to the end of the journal
   * journal can only be appended in the back,
   * no modification of existing frames is allowed. */
  void seekEnd();
  /** to write a string into journal */
  int64_t write_str(const string& str, FH_TYPE_MSG_TP msgType);

  /** write a frame with full information */
  int64_t write_frame(const void* data, FH_TYPE_LENGTH length, FH_TYPE_MSG_TP msgType,
                      FH_TYPE_LASTFG lastFlag);

  template <typename T>
  inline int64_t write_data(const T* data, FH_TYPE_MSG_TP msgType, FH_TYPE_LASTFG lastFlag) {
    return write_frame(data, sizeof(T), msgType, lastFlag);
  }
  template <typename T>
  inline int64_t write_data(const T& data, FH_TYPE_MSG_TP msgType, FH_TYPE_LASTFG lastFlag) {
    return write_frame(&data, sizeof(T), msgType, lastFlag);
  }

  /*data is copied to frame from elsewhere (may avoid double copy where preparing data)*/
  /** get next writable frame address */
  Frame locateFrame();
  /** move forward to next frame */
  void passFrame(Frame& frame, FH_TYPE_LENGTH length, FH_TYPE_MSG_TP msgType,
                 FH_TYPE_LASTFG lastFlag);

 public:
  // creators
  static JournalWriterPtr create(const string& dir, const string& jname, const string& writerName);
  static JournalWriterPtr create(const string& dir, const string& jname, PageProviderPtr& ptr);

 public:
  //    static const string PREFIX;
};

YJJ_NAMESPACE_END
#endif  // YIJINJING_JOURNALWRITER_H
