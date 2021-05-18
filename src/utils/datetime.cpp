// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/utils/datetime.h"

#include <cctype>
#include <stdexcept>

#include "fmt/format.h"

namespace ft::datetime {

constexpr int kDayRangeOfLeapYear[] = {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr int kDayRangeOfNonLeapYear[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

constexpr int kAccumulatedDayOfLeapYear[] = {0,   0,   31,  60,  91,  121, 152,
                                             182, 213, 244, 274, 305, 335};
constexpr int kAccumulatedDayOfNonLeapYear[] = {0,   0,   31,  59,  90,  120, 151,
                                                181, 212, 243, 273, 304, 334};

namespace {
void FastUnixSecondToDate(std::time_t unix_sec, std::tm* tm) {
  static constexpr int kTimeZone = 8;
  static constexpr int kHoursInDay = 24;
  static constexpr int kMinutesInHour = 60;
  static constexpr int kDaysFromUnixTime = 2472632;
  static constexpr int kDaysFromYear = 153;
  static constexpr int kMagicUnkonwnFirst = 146097;
  static constexpr int kMagicUnkonwnSec = 1461;

  tm->tm_sec = unix_sec % kMinutesInHour;
  int i = (unix_sec / kMinutesInHour);
  tm->tm_min = i % kMinutesInHour;  // nn
  i /= kMinutesInHour;
  tm->tm_hour = (i + kTimeZone) % kHoursInDay;  // hh
  tm->tm_mday = (i + kTimeZone) / kHoursInDay;
  int a = tm->tm_mday + kDaysFromUnixTime;
  int b = (a * 4 + 3) / kMagicUnkonwnFirst;
  int c = (-b * kMagicUnkonwnFirst) / 4 + a;
  int d = ((c * 4 + 3) / kMagicUnkonwnSec);
  int e = -d * kMagicUnkonwnSec;
  e = e / 4 + c;
  int m = (5 * e + 2) / kDaysFromYear;

  tm->tm_mday = -(kDaysFromYear * m + 2) / 5 + e + 1;
  tm->tm_mon = (-m / 10) * 12 + m + 2;
  tm->tm_year = b * 100 + d - 6700 + (m / 10);
}
}  // namespace

Timedelta::Timedelta(int days, int hours, int minutes) : days_(days) {
  hours += time_carry(minutes, 60);

  days_ += time_carry(hours, 24);
  seconds_ = time_mod(minutes, 60) * 60 + time_mod(hours, 24) * 3600;
}

Timedelta::Timedelta(int days, int hours, int minutes, int seconds) : days_(days) {
  minutes += time_carry(seconds, 60);
  hours += time_carry(minutes, 60);

  days_ += time_carry(hours, 24);
  seconds_ = time_mod(seconds, 60) + time_mod(minutes, 60) * 60 + time_mod(hours, 24) * 3600;
}

Timedelta::Timedelta(int days, int hours, int minutes, int seconds, int milliseconds)
    : days_(days) {
  seconds += time_carry(milliseconds, 1000);
  minutes += time_carry(seconds, 60);
  hours += time_carry(minutes, 60);

  days_ += time_carry(hours, 24);
  seconds_ = time_mod(seconds, 60) + time_mod(minutes, 60) * 60 + time_mod(hours, 24) * 3600;
  microseconds_ = time_mod(milliseconds, 1000) * 1000;
}

Timedelta::Timedelta(int days, int hours, int minutes, int seconds, int milliseconds,
                     int microseconds)
    : days_(days) {
  milliseconds += time_carry(microseconds, 1000);
  seconds += time_carry(milliseconds, 1000);
  minutes += time_carry(seconds, 60);
  hours += time_carry(minutes, 60);

  days_ += time_carry(hours, 24);
  seconds_ = time_mod(seconds, 60) + time_mod(minutes, 60) * 60 + time_mod(hours, 24) * 3600;
  microseconds_ = time_mod(microseconds, 1000) + time_mod(milliseconds, 1000) * 1000;
}

Date::Date(int year, int month, int day)
    : year_(year), month_(month), day_(day), is_leap_year_(false) {
  const int* day_range_table = kDayRangeOfNonLeapYear;
  if (::ft::datetime::is_leap_year(year)) {
    is_leap_year_ = true;
    day_range_table = kDayRangeOfLeapYear;
  }

  if (year > kMaxYear || year < kMinYear || month <= 0 || month > 12 || day <= 0 ||
      day > day_range_table[month]) {
    throw std::out_of_range(fmt::format("date out of range: {}/{}/{}", year, month, day));
  }

  integer_date_ = (year_ << 9) | (month_ << 4) | day_;
}

Date Date::operator+(const Timedelta& timedelta) const {
  int delta_days = timedelta.days();
  if (delta_days < 0) {
    return *this - Timedelta(-delta_days);
  }

  if (delta_days == 0) {
    return *this;
  }

  int dst_year = year_;
  int dst_month = month_;
  int dst_day = day_;

calculate:
  int total_days_this_year = 365;
  const int* day_range_table = kDayRangeOfNonLeapYear;
  const int* acc_days_table = kAccumulatedDayOfNonLeapYear;
  if (::ft::datetime::is_leap_year(dst_year)) {
    total_days_this_year = 366;
    day_range_table = kDayRangeOfLeapYear;
    acc_days_table = kAccumulatedDayOfLeapYear;
  }

  int left_days_this_year = total_days_this_year - (acc_days_table[dst_month] + dst_day);
  if (delta_days <= left_days_this_year) {
    for (;;) {
      int left_days_this_month = day_range_table[dst_month] - dst_day;
      if (delta_days <= left_days_this_month) {
        dst_day += delta_days;
        break;
      } else {
        delta_days -= left_days_this_month + 1;
        ++dst_month;
        dst_day = 1;
      }
    }
  } else {
    delta_days -= left_days_this_year + 1;
    ++dst_year;
    dst_month = 1;
    dst_day = 1;
    goto calculate;
  }

  return Date(dst_year, dst_month, dst_day);
}

Date Date::operator-(const Timedelta& timedelta) const {
  int delta_days = timedelta.days();
  if (delta_days < 0) {
    return *this + Timedelta(-delta_days);
  }

  if (delta_days == 0) {
    return *this;
  }

  int dst_year = year_;
  int dst_month = month_;
  int dst_day = day_;

calculate:
  const int* day_range_table = kDayRangeOfNonLeapYear;
  const int* acc_days_table = kAccumulatedDayOfNonLeapYear;
  if (::ft::datetime::is_leap_year(dst_year)) {
    day_range_table = kDayRangeOfLeapYear;
    acc_days_table = kAccumulatedDayOfLeapYear;
  }

  int days_this_year = acc_days_table[dst_month] + dst_day - 1;
  if (delta_days <= days_this_year) {
    for (;;) {
      if (delta_days < dst_day) {
        dst_day -= delta_days;
        break;
      } else {
        delta_days -= dst_day;
        --dst_month;
        dst_day = day_range_table[dst_month];
      }
    }
  } else {
    delta_days -= days_this_year + 1;
    --dst_year;
    dst_month = 12;
    dst_day = 31;
    goto calculate;
  }

  return Date(dst_year, dst_month, dst_day);
}

Timedelta Date::operator-(const Date& rhs) const {
  if (year_ == rhs.year_) {
    const int* acc_days_table = kAccumulatedDayOfNonLeapYear;
    if (is_leap_year_) {
      acc_days_table = kAccumulatedDayOfLeapYear;
    }
    return Timedelta(acc_days_table[month_] + day_ - acc_days_table[rhs.month_] - rhs.day_);
  } else {
    const Date* early_date = &rhs;
    const Date* latter_date = this;
    if (rhs.year_ > year_) {
      early_date = this;
      latter_date = &rhs;
    }

    Timedelta timedelta(1);
    timedelta += Date(early_date->year_, 12, 31) - (*early_date);
    timedelta += (*latter_date) - Date(latter_date->year_, 1, 1);

    int y = early_date->year_ + 1;
    while (y != latter_date->year_) {
      if (::ft::datetime::is_leap_year(y)) {
        timedelta += Timedelta(366);
      } else {
        timedelta += Timedelta(365);
      }
      ++y;
    }

    if (early_date == this) {
      return Timedelta(-timedelta.days());
    }
    return timedelta;
  }
}

Date Date::today() {
  std::tm tm;
  std::time_t unix_sec = std::time(nullptr);
  FastUnixSecondToDate(unix_sec, &tm);

  return Date(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

Time::Time(int hour, int minute, int second, int microsecond)
    : hour_(hour), minute_(minute), second_(second), microsecond_(microsecond) {
  if (hour_ < 0 || hour_ > 23 || minute_ < 0 || minute_ > 59 || second_ < 0 || second_ > 59 ||
      microsecond_ < 0 || microsecond_ > 999999) {
    throw std::out_of_range(
        fmt::format("time out of range: {}:{}:{}.{}", hour_, minute_, second_, microsecond_));
  }

  microsecond_count_ =
      microsecond_ + second_ * 1000000LL + minute_ * 60LL * 1000000LL + hour_ * 3600LL * 1000000LL;
}

Datetime Datetime::operator+(const Timedelta& timedelta) const {
  int dst_microsecond = time_.microsecond();
  int dst_second = time_.second();
  int dst_minute = time_.minute();
  int dst_hour = time_.hour();

  int delta_microseconds = timedelta.microseconds();
  int delta_seconds = timedelta.seconds();

  dst_microsecond += delta_microseconds;
  dst_second += delta_seconds + dst_microsecond / 1000000;
  dst_minute += dst_second / 60;
  dst_hour += dst_minute / 60;

  Time dst_time(dst_hour % 24, dst_minute % 60, dst_second % 60, dst_microsecond % 1000000);

  int day_moved = dst_hour / 24;
  if (day_moved == 0) {
    return Datetime(date_, dst_time);
  } else {
    Date dst_date = date_ + Timedelta(timedelta.days() + day_moved);
    return Datetime(dst_date, dst_time);
  }
}

Datetime Datetime::operator-(const Timedelta& timedelta) const {
  return (*this + Timedelta(-timedelta.days(), -timedelta.seconds(), -timedelta.microseconds()));
}

Timedelta Datetime::operator-(const Datetime& rhs) const {
  return (date_ - rhs.date_) + Timedelta(0, hour() - rhs.hour(), minute() - rhs.minute(),
                                         second() - rhs.second(), 0,
                                         microsecond() - rhs.microsecond());
}

Datetime Datetime::today() {
  std::tm tm;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  FastUnixSecondToDate(ts.tv_sec, &tm);

  return Datetime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                  ts.tv_nsec / 1000);
}

std::string Timedelta::ToString() const {
  return fmt::format("ft::datetime::Timedelta({}, {}, {})", days_, seconds_, microseconds_);
}

std::string Date::ToString() const {
  return fmt::format("ft::datetime::Date({}, {}, {})", year_, month_, day_);
}

std::string Time::ToString() const {
  return fmt::format("ft::datetime::Time({}, {}, {}, {})", hour_, minute_, second_, microsecond_);
}

std::string Datetime::ToString() const {
  return fmt::format("ft::datetime::Datetime({}, {}, {}, {}, {}, {}, {})", date_.year(),
                     date_.month(), date_.day(), time_.hour(), time_.minute(), time_.second(),
                     time_.microsecond());
}

Datetime strptime(const std::string& str, const std::string& fmt) {
  const char* pfmt = fmt.c_str();
  const char* pstr = str.c_str();

  bool find_year = false;
  bool find_month = false;
  bool find_day = false;
  bool find_hour = false;
  bool find_minute = false;
  bool find_second = false;
  bool find_millisecond = false;
  bool find_microsecond = false;

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int millisecond = 0;
  int microsecond = 0;

  while (*pfmt && *pstr) {
    if (*pfmt != '%') {
      if (*pfmt != *pstr) {
        goto error;
      }
      ++pfmt;
      ++pstr;
      continue;
    }

    ++pfmt;
    switch (*pfmt) {
      case 'Y': {
        if (find_year || pstr[0] == 0 || pstr[1] == 0 || pstr[2] == 0 || pstr[3] == 0 ||
            !isdigit(pstr[0]) || !isdigit(pstr[1]) || !isdigit(pstr[2]) || !isdigit(pstr[3])) {
          goto error;
        }
        year = static_cast<int>(pstr[0] - '0') * 1000 + static_cast<int>(pstr[1] - '0') * 100 +
               static_cast<int>(pstr[2] - '0') * 10 + static_cast<int>(pstr[3] - '0');
        pstr += 4;
        ++pfmt;
        find_year = true;
        break;
      }
      case 'm': {
        if (find_month || pstr[0] == 0 || pstr[1] == 0 || !isdigit(pstr[0]) || !isdigit(pstr[1])) {
          goto error;
        }
        month = static_cast<int>(pstr[0] - '0') * 10 + static_cast<int>(pstr[1] - '0');
        pstr += 2;
        ++pfmt;
        find_month = true;
        break;
      }
      case 'd': {
        if (find_day || pstr[0] == 0 || pstr[1] == 0 || !isdigit(pstr[0]) || !isdigit(pstr[1])) {
          goto error;
        }
        day = static_cast<int>(pstr[0] - '0') * 10 + static_cast<int>(pstr[1] - '0');
        pstr += 2;
        ++pfmt;
        find_day = true;
        break;
      }
      case 'H': {
        if (find_hour || pstr[0] == 0 || pstr[1] == 0 || !isdigit(pstr[0]) || !isdigit(pstr[1])) {
          goto error;
        }
        hour = static_cast<int>(pstr[0] - '0') * 10 + static_cast<int>(pstr[1] - '0');
        pstr += 2;
        ++pfmt;
        find_hour = true;
        break;
      }
      case 'M': {
        if (find_minute || pstr[0] == 0 || pstr[1] == 0 || !isdigit(pstr[0]) || !isdigit(pstr[1])) {
          goto error;
        }
        minute = static_cast<int>(pstr[0] - '0') * 10 + static_cast<int>(pstr[1] - '0');
        pstr += 2;
        ++pfmt;
        find_minute = true;
        break;
      }
      case 'S': {
        if (find_second || pstr[0] == 0 || pstr[1] == 0 || !isdigit(pstr[0]) || !isdigit(pstr[1])) {
          goto error;
        }
        second = static_cast<int>(pstr[0] - '0') * 10 + static_cast<int>(pstr[1] - '0');
        pstr += 2;
        ++pfmt;
        find_second = true;
        break;
      }
      case 's': {
        if (find_millisecond || pstr[0] == 0 || pstr[1] == 0 || pstr[2] == 0 || !isdigit(pstr[0]) ||
            !isdigit(pstr[1]) || !isdigit(pstr[2])) {
          goto error;
        }
        millisecond = static_cast<int>(pstr[0] - '0') * 100 + static_cast<int>(pstr[1] - '0') * 10 +
                      static_cast<int>(pstr[2] - '0');
        pstr += 3;
        ++pfmt;
        find_millisecond = true;
        break;
      }
      case 'u': {
        if (find_microsecond || pstr[0] == 0 || pstr[1] == 0 || pstr[2] == 0 || !isdigit(pstr[0]) ||
            !isdigit(pstr[1]) || !isdigit(pstr[2])) {
          goto error;
        }
        microsecond = static_cast<int>(pstr[0] - '0') * 100 + static_cast<int>(pstr[1] - '0') * 10 +
                      static_cast<int>(pstr[2] - '0');
        pstr += 3;
        ++pfmt;
        find_microsecond = true;
        break;
      }
      case '%': {
        if (*pstr == 0 || *pstr != '%') {
          goto error;
        }
        ++pstr;
        ++pfmt;
        break;
      }
      default: {
        assert(false);
      }
    }
  }

  if (*pfmt != 0) {
    goto error;
  }
  return Datetime(year, month, day, hour, minute, second, 1000 * millisecond + microsecond);

error:
  throw std::runtime_error("ft::datetime::strptime: invalid format");
}

}  // namespace ft::datetime
