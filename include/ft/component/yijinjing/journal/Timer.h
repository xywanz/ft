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
 * Timer for kungfu system.
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   March, 2017
 * Provide basic nano time and time transformation
 */

#ifndef YIJINJING_TIMER_H
#define YIJINJING_TIMER_H

#include "ft/component/yijinjing/utils/YJJ_DECLARE.h"

YJJ_NAMESPACE_START

const int64_t MILLISECONDS_PER_SECOND = 1000;
const int64_t MICROSECONDS_PER_MILLISECOND = 1000;
const int64_t NANOSECONDS_PER_MICROSECOND = 1000;

const int64_t MICROSECONDS_PER_SECOND = MICROSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
const int64_t NANOSECONDS_PER_MILLISECOND =
    NANOSECONDS_PER_MICROSECOND * MICROSECONDS_PER_MILLISECOND;
const int64_t NANOSECONDS_PER_SECOND = NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;

const int SECONDS_PER_MINUTE = 60;
const int MINUTES_PER_HOUR = 60;
const int HOURS_PER_DAY = 24;
const int SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;

const int64_t MILLISECONDS_PER_MINUTE = MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE;
const int64_t NANOSECONDS_PER_MINUTE = NANOSECONDS_PER_SECOND * SECONDS_PER_MINUTE;
const int64_t NANOSECONDS_PER_HOUR = NANOSECONDS_PER_SECOND * SECONDS_PER_HOUR;
const int64_t NANOSECONDS_PER_DAY = NANOSECONDS_PER_HOUR * HOURS_PER_DAY;

/**
 * timer for nanosecond, main class
 */
class NanoTimer {
 public:
  /** return current nano time: unix-timestamp * 1e9 + nano-part */
  int64_t getNano() const;
  /** return secDiff */
  inline int64_t getSecDiff() const { return secDiff; }
  /** singleton */
  static NanoTimer *getInstance();

 private:
  NanoTimer();
  /** singleton */
  static std::shared_ptr<NanoTimer> m_ptr;
  /** object to be updated every time called */
  int64_t secDiff;
};

/**
 * util function to utilize NanoTimer
 * @return current nano time in int64_t (unix-timestamp * 1e9 + nano-part)
 */
inline int64_t getNanoTime() { return NanoTimer::getInstance()->getNano(); }

/**
 * util function to utilize NanoTimer
 * @return second diff in int64_t for nano time matching
 */
inline int64_t getSecDiff() { return NanoTimer::getInstance()->getSecDiff(); }

/**
 * parse struct tm to nano time
 * @param _tm ctime struct
 * @return nano time in int64_t
 */
inline int64_t parseTm(struct tm _tm) {
#ifdef _WINDOWS
  return mktime(&_tm) * NANOSECONDS_PER_SECOND;
#else
  return timelocal(&_tm) * NANOSECONDS_PER_SECOND;
#endif  // _WINDOWS
        /// return timelocal(&_tm) * NANOSECONDS_PER_SECOND;
}

/**
 * parse string time to nano time
 * @param timeStr string-formatted time
 * @param format eg: %Y%m%d-%H:%M:%S
 * @return nano time in int64_t
 */
inline int64_t parseTime(const char *timeStr, const char *format) {
  struct tm _tm;
  strptime(timeStr, format, &_tm);
  return parseTm(_tm);
}

/**
 * dump int64_t time to string with format
 * @param nano nano time in int64_t
 * @param format eg: %Y%m%d-%H:%M:%S
 * @return string-formatted time
 */
inline string parseNano(int64_t nano, const char *format) {
  if (nano <= 0) return string("NULL");
  time_t seconds = nano / NANOSECONDS_PER_SECOND;
  struct tm *dt;
  char buffer[30];
  dt = localtime((const time_t *)&seconds);
  strftime(buffer, sizeof(buffer), format, dt);
  return string(buffer);
}

/**
 * dump int64_t time to struct tm
 * @param nano nano time in int64_t
 * @return ctime struct
 */
inline struct tm parseNano(int64_t nano) {
  time_t sec_num = nano / NANOSECONDS_PER_SECOND;
  return *localtime(&sec_num);
}

YJJ_NAMESPACE_END

#endif  // YIJINJING_TIMER_H