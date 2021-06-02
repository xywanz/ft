// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_DATETIME_H_
#define FT_INCLUDE_FT_UTILS_DATETIME_H_

#include <cassert>
#include <cstdint>
#include <ctime>
#include <string>

namespace ft::datetime {

constexpr int kMaxYear = 9999;
constexpr int kMinYear = 1;

// 时间进位或借位
// 主要目的在于处理时间为负数的情况，把负数时间规范化
// 如果时间为负数，则通过借位的方式把其置为正数
// 以分钟和小时为例
// time_carry(-1, 60) == -1    借一个小时
// time_carry(-61, 60) == -2   借两个小时
inline int time_carry(int time, int base) {
  assert(base > 0);
  return time < 0 ? ((time + 1) / base - 1) : (time / base);
}

// 时间进位或借位后的值
// 主要目的在于处理时间为负数的情况，把负数时间规范化
// 如果时间为负数，则通过借位的方式把负数补成正数
// 以分钟和小时为例
// time_mod(-1, 60) == 59        -1 + 60 == 59
// time_mod(-61, 60) == 59       -1 + 60 + 60 = 59
inline int time_mod(int time, int base) {
  assert(base > 0);
  return time < 0 ? (time % base + base) : (time % base);
}

inline bool is_leap_year(int year) {
  return !((year % 4) != 0 || ((year % 100) == 0 && (year % 400) != 0));
}

class Timedelta {
 public:
  explicit Timedelta(int days = 0) : days_(days) {}

  Timedelta(int days, int hours) : days_(days) {
    days_ += time_carry(hours, 24);
    seconds_ = time_mod(hours, 24) * 3600;
  }

  // TODO(Kevin): 加上溢出检查
  Timedelta(int days, int hours, int minutes);
  Timedelta(int days, int hours, int minutes, int seconds);
  Timedelta(int days, int hours, int minutes, int seconds, int milliseconds);
  Timedelta(int days, int hours, int minutes, int seconds, int milliseconds, int microseconds);

  int days() const { return days_; }
  int seconds() const { return seconds_; }
  int microseconds() const { return microseconds_; }

  bool operator==(const Timedelta& rhs) const {
    return days_ == rhs.days_ && seconds_ == rhs.seconds_ && microseconds_ == rhs.microseconds_;
  }

  bool operator!=(const Timedelta& rhs) const { return !(*this == rhs); }

  bool operator>=(const Timedelta& rhs) const {
    return microsecond_count() >= rhs.microsecond_count();
  }

  bool operator>(const Timedelta& rhs) const {
    return microsecond_count() > rhs.microsecond_count();
  }

  bool operator<=(const Timedelta& rhs) const { return !(*this > rhs); }

  bool operator<(const Timedelta& rhs) const { return !(*this >= rhs); }

  Timedelta operator+(const Timedelta& rhs) const {
    return Timedelta(days_ + rhs.days_, 0, 0, seconds_ + rhs.seconds_, 0,
                     microseconds_ + rhs.microseconds_);
  }

  Timedelta operator-(const Timedelta& rhs) const {
    return Timedelta(days_ - rhs.days_, 0, 0, seconds_ - rhs.seconds_, 0,
                     microseconds_ - rhs.microseconds_);
  }

  Timedelta& operator+=(const Timedelta& rhs) {
    *this = *this + rhs;
    return *this;
  }

  Timedelta& operator-=(const Timedelta& rhs) {
    *this = *this - rhs;
    return *this;
  }

  std::string ToString() const;

 private:
  int64_t microsecond_count() const {
    return microseconds_ + seconds_ * 1000000LL + days_ * 24LL * 3600LL * 1000000LL;
  }

 private:
  int days_ = 0;
  int seconds_ = 0;
  int microseconds_ = 0;
};

class Date {
 public:
  explicit Date(int year = 1, int month = 1, int day = 1);

  int year() const { return year_; }
  int month() const { return month_; }
  int day() const { return day_; }
  bool is_leap_year() const { return is_leap_year_; }

  bool operator==(const Date& rhs) const { return integer_date_ == rhs.integer_date_; }
  bool operator!=(const Date& rhs) const { return integer_date_ != rhs.integer_date_; }
  bool operator>=(const Date& rhs) const { return integer_date_ >= rhs.integer_date_; }
  bool operator>(const Date& rhs) const { return integer_date_ > rhs.integer_date_; }
  bool operator<=(const Date& rhs) const { return integer_date_ <= rhs.integer_date_; }
  bool operator<(const Date& rhs) const { return integer_date_ < rhs.integer_date_; }

  Date operator+(const Timedelta& timedelta) const;
  Date operator-(const Timedelta& timedelta) const;

  Date& operator+=(const Timedelta& timedelta) {
    *this = *this + timedelta;
    return *this;
  }

  Date& operator-=(const Timedelta& timedelta) {
    *this = *this - timedelta;
    return *this;
  }

  Timedelta operator-(const Date& rhs) const;

  std::string ToString() const;

 public:
  static Date today();

  static Date max() { return Date(kMaxYear, 12, 31); }
  static Date min() { return Date(kMinYear, 1, 1); }

 private:
  int year_;
  int month_;
  int day_;

  bool is_leap_year_;
  int integer_date_;  // YYYYmmdd
};

class Time {
 public:
  explicit Time(int hour = 0, int minute = 0, int second = 0, int microsecond = 0);

  int hour() const { return hour_; }
  int minute() const { return minute_; }
  int second() const { return second_; }
  int microsecond() const { return microsecond_; }

  bool operator==(const Time& rhs) const { return microsecond_count_ == rhs.microsecond_count_; }
  bool operator!=(const Time& rhs) const { return microsecond_count_ != rhs.microsecond_count_; }
  bool operator>=(const Time& rhs) const { return microsecond_count_ >= rhs.microsecond_count_; }
  bool operator>(const Time& rhs) const { return microsecond_count_ > rhs.microsecond_count_; }
  bool operator<=(const Time& rhs) const { return microsecond_count_ <= rhs.microsecond_count_; }
  bool operator<(const Time& rhs) const { return microsecond_count_ < rhs.microsecond_count_; }

  std::string ToString() const;

 public:
  static Time max() { return Time(23, 59, 59, 999999); }
  static Time min() { return Time(0, 0, 0, 0); }
  static Timedelta resolution() { return Timedelta(0, 0, 0, 0, 0, 1); }

 private:
  int hour_ = 0;
  int minute_ = 0;
  int second_ = 0;
  int microsecond_ = 0;

  int64_t microsecond_count_ = 0;
};

class Datetime {
 public:
  Datetime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0,
           int microsecond = 0)
      : date_(year, month, day), time_(hour, minute, second, microsecond) {}

  int year() const { return date_.year(); }
  int month() const { return date_.month(); }
  int day() const { return date_.day(); }
  int hour() const { return time_.hour(); }
  int minute() const { return time_.minute(); }
  int second() const { return time_.second(); }
  int microsecond() const { return time_.microsecond(); }

  bool operator==(const Datetime& rhs) const { return date_ == rhs.date_ && time_ == rhs.time_; }
  bool operator!=(const Datetime& rhs) const { return !(*this == rhs); }
  bool operator>=(const Datetime& rhs) const {
    return date_ > rhs.date_ || (date_ == rhs.date_ && time_ >= rhs.time_);
  }
  bool operator<=(const Datetime& rhs) const {
    return date_ < rhs.date_ || (date_ == rhs.date_ && time_ <= rhs.time_);
  }
  bool operator>(const Datetime& rhs) const { return !(*this <= rhs); }
  bool operator<(const Datetime& rhs) const { return !(*this >= rhs); }

  Datetime operator+(const Timedelta& timedelta) const;
  Datetime operator-(const Timedelta& timedelta) const;

  Datetime& operator+=(const Timedelta& timedelta) {
    *this = *this + timedelta;
    return *this;
  }

  Datetime& operator-=(const Timedelta& timedelta) {
    *this = *this - timedelta;
    return *this;
  }

  Timedelta operator-(const Datetime& rhs) const;

  std::string ToString() const;

 public:
  static Datetime today();
  static Datetime now() { return today(); }

 private:
  Datetime(const Date& date, const Time& time) : date_(date), time_(time) {}

 private:
  Date date_{};
  Time time_{};
};

// 将格式化的字符串转化为Datetime，支持微秒级别的精度
// 格式化符号:
//     %Y 四位数表示的年份 (0001-9999)
//     %m 两位数表示的月份 (01-12)
//     %d 两位数表示的月份中的一天 (01-31)
//     %H 两位数表示的小时数 (00-23)
//     %M 两位数表示的分钟数 (00-59)
//     %S 两位数表示的秒钟数 (00-59)
//     %s 三位数表示的毫秒数 (000-999)
//     %u 三位数表示的微秒数 (000-999)
//     %% 百分号
//
// 示例
// auto dt = strptime("2021/02/11 15:30:55.865302", "%Y/%m/%d %H:%M:%S.%s%u");
// std::cout << dt.ToString() << std::endl;  // Datetime(2021, 2, 11, 15, 30, 55, 865302)
//
// 转换失败将会抛出错误
Datetime strptime(const std::string& str, const std::string& fmt);

}  // namespace ft::datetime

#endif  // FT_INCLUDE_FT_UTILS_DATETIME_H_
