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
 * Frame.
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   March, 2017
 * Basic memory unit in yjj
 */

#ifndef YIJINJING_FRAME_HPP
#define YIJINJING_FRAME_HPP

#include <string.h>  // memcpy

#include "ft/component/yijinjing/journal/FrameHeader.h"
#include "ft/component/yijinjing/utils/YJJ_DECLARE.h"

#ifdef FRAME_AUTO_SET_HASHCODE
#include "ft/component/yijinjing/utils/Hash.hpp"
#endif

YJJ_NAMESPACE_START

/**
 * Basic memory unit,
 * holds header / data / errorMsg (if needs)
 */
class Frame {
 private:
  /** address with type,
   * will keep moving forward until change page */
  FrameHeader* frame;

 public:
  /** default constructor */
  Frame(void*);
  /** setup basic frame address */
  void set_address(void*);
  /** get current address */
  void* get_address() const;

  // get fields of header from frame
  FH_TYPE_STATUS getStatus() const;
  FH_TYPE_NANOTM getNano() const;
  FH_TYPE_LENGTH getFrameLength() const;
  FH_TYPE_LENGTH getHeaderLength() const;
  FH_TYPE_LENGTH getDataLength() const;
  FH_TYPE_HASHNM getHashCode() const;
  FH_TYPE_MSG_TP getMsgType() const;
  FH_TYPE_LASTFG getLastFlag() const;

  /** get address of data field */
  void* getData() const;
  /** parse data content as string with char */
  string getStr() const;

  void setNano(FH_TYPE_NANOTM);
  void setMsgType(FH_TYPE_MSG_TP);
  void setLastFlag(FH_TYPE_LASTFG);
  /** set data with length */
  void setData(const void*, size_t);
  /** set data  length */
  void setDataLength(size_t);
  /** mark status as written */
  void setStatusWritten();
  /** mark status as page closed */
  void setStatusPageClosed();
  /** move the frame forward by length */
  FH_TYPE_LENGTH next();

 private:
  /** return address of next frame header */
  FrameHeader* getNextEntry() const;
  /** set status, internally used */
  void setStatus(FH_TYPE_STATUS);
  /** set hash code, internally used */
  void setHashCode(FH_TYPE_HASHNM);
  /** set length, internally used */
  void setFrameLength(FH_TYPE_LENGTH);
};

DECLARE_PTR(Frame);

inline Frame::Frame(void* fm) { set_address(fm); }

inline void Frame::set_address(void* fm) { frame = (FrameHeader*)fm; }

inline void* Frame::get_address() const { return frame; }

inline FH_TYPE_LENGTH Frame::getHeaderLength() const { return BASIC_FRAME_HEADER_LENGTH; }

inline FH_TYPE_STATUS Frame::getStatus() const { return frame->status; }
inline FH_TYPE_NANOTM Frame::getNano() const { return frame->nano; }

inline FH_TYPE_LENGTH Frame::getFrameLength() const { return frame->length; }

inline FH_TYPE_HASHNM Frame::getHashCode() const { return frame->hash; }

inline FH_TYPE_MSG_TP Frame::getMsgType() const { return frame->msg_type; }

inline FH_TYPE_LASTFG Frame::getLastFlag() const { return frame->last_flag; }

inline FH_TYPE_LENGTH Frame::getDataLength() const {
  return getFrameLength() - BASIC_FRAME_HEADER_LENGTH;
}

inline void* Frame::getData() const { return ADDRESS_ADD(frame, BASIC_FRAME_HEADER_LENGTH); }

inline string Frame::getStr() const { return string((char*)getData(), getDataLength()); }

inline void Frame::setStatus(FH_TYPE_STATUS status) { frame->status = status; }

inline void Frame::setNano(FH_TYPE_NANOTM nano) { frame->nano = nano; }

inline void Frame::setFrameLength(FH_TYPE_LENGTH length) { frame->length = length; }

inline void Frame::setHashCode(FH_TYPE_HASHNM hashCode) { frame->hash = hashCode; }

inline void Frame::setMsgType(FH_TYPE_MSG_TP msgType) { frame->msg_type = msgType; }

inline void Frame::setLastFlag(FH_TYPE_LASTFG lastFlag) { frame->last_flag = lastFlag; }

inline void Frame::setData(const void* data, size_t dataLength) {
  memcpy(ADDRESS_ADD(frame, BASIC_FRAME_HEADER_LENGTH), data, dataLength);
  setFrameLength(BASIC_FRAME_HEADER_LENGTH + dataLength);
#ifdef FRAME_AUTO_SET_HASHCODE
  setHashCode(MurmurHash2(data, dataLength, HASH_SEED));
#endif
}

inline void Frame::setDataLength(size_t dataLength) {
  setFrameLength(BASIC_FRAME_HEADER_LENGTH + dataLength);
#ifdef FRAME_AUTO_SET_HASHCODE
  setHashCode(MurmurHash2(getData(), dataLength, HASH_SEED));
#endif
}

inline void Frame::setStatusWritten() {
  /** just make sure next frame won't be wrongly read */
  getNextEntry()->status = JOURNAL_FRAME_STATUS_RAW;
  setStatus(JOURNAL_FRAME_STATUS_WRITTEN);
}

inline void Frame::setStatusPageClosed() { frame->status = JOURNAL_FRAME_STATUS_PAGE_END; }

inline FH_TYPE_LENGTH Frame::next() {
  FH_TYPE_LENGTH len = getFrameLength();
  frame = getNextEntry();
  return len;
}

inline FrameHeader* Frame::getNextEntry() const {
  return (FrameHeader*)ADDRESS_ADD(frame, getFrameLength());
}

YJJ_NAMESPACE_END

#endif  // YIJINJING_FRAME_HPP
