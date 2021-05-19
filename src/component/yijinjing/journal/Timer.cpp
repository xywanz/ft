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

#include "ft/component/yijinjing/journal/Timer.h"

#include <array>
#include <chrono>

#include "ft/component/yijinjing/journal/PageProvider.h"

USING_YJJ_NAMESPACE

std::shared_ptr<NanoTimer> NanoTimer::m_ptr = std::shared_ptr<NanoTimer>(nullptr);

NanoTimer *NanoTimer::getInstance() {
  if (m_ptr.get() == nullptr) {
    m_ptr = std::shared_ptr<NanoTimer>(new NanoTimer());
  }
  return m_ptr.get();
}

inline std::chrono::steady_clock::time_point get_time_now() {
#if defined __linux__
  timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return std::chrono::steady_clock::time_point(std::chrono::steady_clock::duration(
      std::chrono::seconds(tp.tv_sec) + std::chrono::nanoseconds(tp.tv_nsec)));
#else
  return std::chrono::steady_clock::now();
#endif
}

inline int64_t get_local_diff() {
  int unix_second_num = std::chrono::seconds(std::time(NULL)).count();
  int tick_second_num =
      std::chrono::duration_cast<std::chrono::seconds>(get_time_now().time_since_epoch()).count();
  return (unix_second_num - tick_second_num) * NANOSECONDS_PER_SECOND;
}

NanoTimer::NanoTimer() { secDiff = get_local_diff(); }

int64_t NanoTimer::getNano() const {
  int64_t _nano =
      std::chrono::duration_cast<std::chrono::nanoseconds>(get_time_now().time_since_epoch())
          .count();
  return _nano + secDiff;
}
