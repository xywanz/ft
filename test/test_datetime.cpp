// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <ctime>

#include "ft/utils/datetime.h"
#include "fmt/format.h"

using ft::datetime::Date;
using ft::datetime::Datetime;
using ft::datetime::Time;
using ft::datetime::Timedelta;

TEST(Util, Datetime) {
  Timedelta timedelta(0, 1, 0, 0, 0, 0);
  ASSERT_EQ(timedelta.days(), 0);
  ASSERT_EQ(timedelta.seconds(), 3600);
  ASSERT_EQ(timedelta.microseconds(), 0);

  ASSERT_NO_THROW(Date(100, 2, 28));
  ASSERT_THROW(Date(100, 2, 29), std::out_of_range);

  ASSERT_NO_THROW(Date(2019, 1, 31));
  ASSERT_NO_THROW(Date(2019, 2, 28));
  ASSERT_NO_THROW(Date(2019, 3, 31));
  ASSERT_NO_THROW(Date(2019, 4, 30));
  ASSERT_NO_THROW(Date(2019, 5, 31));
  ASSERT_NO_THROW(Date(2019, 6, 30));
  ASSERT_NO_THROW(Date(2019, 7, 31));
  ASSERT_NO_THROW(Date(2019, 8, 31));
  ASSERT_NO_THROW(Date(2019, 9, 30));
  ASSERT_NO_THROW(Date(2019, 10, 31));
  ASSERT_NO_THROW(Date(2019, 11, 30));
  ASSERT_NO_THROW(Date(2019, 12, 31));

  ASSERT_NO_THROW(Date(2020, 1, 31));
  ASSERT_NO_THROW(Date(2020, 2, 29));
  ASSERT_NO_THROW(Date(2020, 3, 31));
  ASSERT_NO_THROW(Date(2020, 4, 30));
  ASSERT_NO_THROW(Date(2020, 5, 31));
  ASSERT_NO_THROW(Date(2020, 6, 30));
  ASSERT_NO_THROW(Date(2020, 7, 31));
  ASSERT_NO_THROW(Date(2020, 8, 31));
  ASSERT_NO_THROW(Date(2020, 9, 30));
  ASSERT_NO_THROW(Date(2020, 10, 31));
  ASSERT_NO_THROW(Date(2020, 11, 30));
  ASSERT_NO_THROW(Date(2020, 12, 31));

  ASSERT_THROW(Date(2019, 1, 32), std::out_of_range);
  ASSERT_THROW(Date(2019, 2, 29), std::out_of_range);
  ASSERT_THROW(Date(2019, 3, 32), std::out_of_range);
  ASSERT_THROW(Date(2019, 4, 31), std::out_of_range);
  ASSERT_THROW(Date(2019, 5, 32), std::out_of_range);
  ASSERT_THROW(Date(2019, 6, 31), std::out_of_range);
  ASSERT_THROW(Date(2019, 7, 32), std::out_of_range);
  ASSERT_THROW(Date(2019, 8, 32), std::out_of_range);
  ASSERT_THROW(Date(2019, 9, 31), std::out_of_range);
  ASSERT_THROW(Date(2019, 10, 32), std::out_of_range);
  ASSERT_THROW(Date(2019, 11, 31), std::out_of_range);
  ASSERT_THROW(Date(2019, 12, 32), std::out_of_range);

  ASSERT_THROW(Date(2020, 1, 32), std::out_of_range);
  ASSERT_THROW(Date(2020, 2, 30), std::out_of_range);
  ASSERT_THROW(Date(2020, 3, 32), std::out_of_range);
  ASSERT_THROW(Date(2020, 4, 31), std::out_of_range);
  ASSERT_THROW(Date(2020, 5, 32), std::out_of_range);
  ASSERT_THROW(Date(2020, 6, 31), std::out_of_range);
  ASSERT_THROW(Date(2020, 7, 32), std::out_of_range);
  ASSERT_THROW(Date(2020, 8, 32), std::out_of_range);
  ASSERT_THROW(Date(2020, 9, 31), std::out_of_range);
  ASSERT_THROW(Date(2020, 10, 32), std::out_of_range);
  ASSERT_THROW(Date(2020, 11, 31), std::out_of_range);
  ASSERT_THROW(Date(2020, 12, 32), std::out_of_range);

  auto today = Date::today();
  std::time_t time = std::time(nullptr);
  std::tm* now_tm = std::localtime(&time);
  ASSERT_EQ(today.day(), now_tm->tm_mday);
  ASSERT_EQ(today.month(), now_tm->tm_mon + 1);
  ASSERT_EQ(today.year(), now_tm->tm_year + 1900);

  Date date(2021, 2, 9);
  ASSERT_EQ(date + Timedelta(0), Date(2021, 2, 9));
  ASSERT_EQ(date + Timedelta(1), Date(2021, 2, 10));
  ASSERT_EQ(date + Timedelta(2), Date(2021, 2, 11));
  ASSERT_EQ(date + Timedelta(30), Date(2021, 3, 11));
  ASSERT_EQ(date + Timedelta(60), Date(2021, 4, 10));
  ASSERT_EQ(date + Timedelta(90), Date(2021, 5, 10));
  ASSERT_EQ(date + Timedelta(120), Date(2021, 6, 9));
  ASSERT_EQ(date + Timedelta(150), Date(2021, 7, 9));
  ASSERT_EQ(date + Timedelta(180), Date(2021, 8, 8));
  ASSERT_EQ(date + Timedelta(210), Date(2021, 9, 7));
  ASSERT_EQ(date + Timedelta(240), Date(2021, 10, 7));
  ASSERT_EQ(date + Timedelta(270), Date(2021, 11, 6));
  ASSERT_EQ(date + Timedelta(300), Date(2021, 12, 6));
  ASSERT_EQ(date + Timedelta(330), Date(2022, 1, 5));
  ASSERT_EQ(date + Timedelta(360), Date(2022, 2, 4));
  ASSERT_EQ(date + Timedelta(390), Date(2022, 3, 6));
  ASSERT_EQ(date + Timedelta(3390), Date(2030, 5, 23));

  ASSERT_EQ(date - Timedelta(0), Date(2021, 2, 9));
  ASSERT_EQ(date - Timedelta(1), Date(2021, 2, 8));
  ASSERT_EQ(date - Timedelta(2), Date(2021, 2, 7));
  ASSERT_EQ(date - Timedelta(30), Date(2021, 1, 10));
  ASSERT_EQ(date - Timedelta(60), Date(2020, 12, 11));
  ASSERT_EQ(date - Timedelta(90), Date(2020, 11, 11));
  ASSERT_EQ(date - Timedelta(120), Date(2020, 10, 12));
  ASSERT_EQ(date - Timedelta(150), Date(2020, 9, 12));
  ASSERT_EQ(date - Timedelta(180), Date(2020, 8, 13));
  ASSERT_EQ(date - Timedelta(210), Date(2020, 7, 14));
  ASSERT_EQ(date - Timedelta(240), Date(2020, 6, 14));
  ASSERT_EQ(date - Timedelta(270), Date(2020, 5, 15));
  ASSERT_EQ(date - Timedelta(300), Date(2020, 4, 15));
  ASSERT_EQ(date - Timedelta(330), Date(2020, 3, 16));
  ASSERT_EQ(date - Timedelta(360), Date(2020, 2, 15));
  ASSERT_EQ(date - Timedelta(390), Date(2020, 1, 16));
  ASSERT_EQ(date - Timedelta(3390), Date(2011, 10, 30));

  ASSERT_EQ(Date(2021, 2, 9) - date, Timedelta(0));
  ASSERT_EQ(Date(2021, 2, 10) - date, Timedelta(1));
  ASSERT_EQ(Date(2021, 2, 11) - date, Timedelta(2));
  ASSERT_EQ(Date(2021, 3, 11) - date, Timedelta(30));
  ASSERT_EQ(Date(2021, 4, 10) - date, Timedelta(60));
  ASSERT_EQ(Date(2021, 5, 10) - date, Timedelta(90));
  ASSERT_EQ(Date(2021, 6, 9) - date, Timedelta(120));
  ASSERT_EQ(Date(2021, 7, 9) - date, Timedelta(150));
  ASSERT_EQ(Date(2021, 8, 8) - date, Timedelta(180));
  ASSERT_EQ(Date(2021, 9, 7) - date, Timedelta(210));
  ASSERT_EQ(Date(2021, 10, 7) - date, Timedelta(240));
  ASSERT_EQ(Date(2021, 11, 6) - date, Timedelta(270));
  ASSERT_EQ(Date(2021, 12, 6) - date, Timedelta(300));
  ASSERT_EQ(Date(2022, 1, 5) - date, Timedelta(330));
  ASSERT_EQ(Date(2022, 2, 4) - date, Timedelta(360));
  ASSERT_EQ(Date(2022, 3, 6) - date, Timedelta(390));
  ASSERT_EQ(Date(2030, 5, 23) - date, Timedelta(3390));

  ASSERT_EQ(date - Date(2021, 2, 8), Timedelta(1));
  ASSERT_EQ(date - Date(2021, 2, 7), Timedelta(2));
  ASSERT_EQ(date - Date(2021, 1, 10), Timedelta(30));
  ASSERT_EQ(date - Date(2020, 12, 11), Timedelta(60));
  ASSERT_EQ(date - Date(2020, 11, 11), Timedelta(90));
  ASSERT_EQ(date - Date(2020, 10, 12), Timedelta(120));
  ASSERT_EQ(date - Date(2020, 9, 12), Timedelta(150));
  ASSERT_EQ(date - Date(2020, 8, 13), Timedelta(180));
  ASSERT_EQ(date - Date(2020, 7, 14), Timedelta(210));
  ASSERT_EQ(date - Date(2020, 6, 14), Timedelta(240));
  ASSERT_EQ(date - Date(2020, 5, 15), Timedelta(270));
  ASSERT_EQ(date - Date(2020, 4, 15), Timedelta(300));
  ASSERT_EQ(date - Date(2020, 3, 16), Timedelta(330));
  ASSERT_EQ(date - Date(2020, 2, 15), Timedelta(360));
  ASSERT_EQ(date - Date(2020, 1, 16), Timedelta(390));
  ASSERT_EQ(date - Date(2011, 10, 30), Timedelta(3390));

  Datetime dt(2021, 2, 10, 18, 48, 58, 100);
  ASSERT_EQ(dt + Timedelta(0, 1, 0, 0, 0, 0), Datetime(2021, 2, 10, 19, 48, 58, 100));
  ASSERT_EQ(dt + Timedelta(1, 8, 8, 88, 888, 888), Datetime(2021, 2, 12, 2, 58, 26, 888988));
  ASSERT_EQ(dt + Timedelta(-888, 888, -888, 888, -888, 888),
            Datetime(2018, 10, 13, 4, 15, 45, 112988));
  ASSERT_EQ(Datetime(2018, 10, 13, 4, 15, 45, 112988) - dt,
            Timedelta(-888, 888, -888, 888, -888, 888));

  ASSERT_EQ(Datetime(2018, 10, 13, 4, 15, 45, 112988) - dt,
            Timedelta(-852, 0, 0, 34007, 0, 112888));

  auto date_from_fmt =
      ft::datetime::strptime("2021/02/11 15:30:55.865302", "%Y/%m/%d %H:%M:%S.%s%u");
  ASSERT_EQ(date_from_fmt, Datetime(2021, 2, 11, 15, 30, 55, 865302));
}
