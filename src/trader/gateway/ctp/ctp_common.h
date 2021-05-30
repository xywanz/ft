// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_COMMON_H_
#define FT_SRC_GATEWAY_CTP_CTP_COMMON_H_

#include <codecvt>
#include <ctime>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ThostFtdcUserApiDataType.h"
#include "ft/base/contract_table.h"
#include "ft/base/trade_msg.h"

namespace ft {

struct CtpApiDeleter {
  template <class T>
  void operator()(T* p) {
    if (p) {
      p->RegisterSpi(nullptr);
      p->Release();
    }
  }
};

class CtpDatetimeConverter {
 public:
  void UpdateDate(TThostFtdcDateType date) {
    if (cur_date_ != date) {
      cur_date_ = date;
      struct tm tmp_tm;
      strptime(date, "%Y%m%d", &tmp_tm);
      time_t t = mktime(&tmp_tm);
      today_timestamp_us_ = t * 1000000UL;
    }
  }

  uint64_t GetExchTimeStamp(TThostFtdcTimeType time, TThostFtdcMillisecType ms) {
    uint64_t hour = (time[0] - '0') * 10UL + (time[1] - '0');
    uint64_t min = (time[3] - '0') * 10UL + (time[4] - '0');
    uint64_t sec = (time[6] - '0') * 10UL + (time[7] - '0');
    return today_timestamp_us_ + hour * 3600000000UL + min * 60000000UL + sec * 1000000UL +
           ms * 1000UL;
  }

 private:
  std::string cur_date_;
  uint64_t today_timestamp_us_ = 0;
};

template <class T>
using CtpUniquePtr = std::unique_ptr<T, CtpApiDeleter>;

inline bool is_error_rsp(CThostFtdcRspInfoField* rsp_info) {
  if (rsp_info && rsp_info->ErrorID != 0) return true;
  return false;
}

inline std::string gb2312_to_utf8(const std::string& gb2312) {
  static const std::locale loc("zh_CN.GB2312");

  std::vector<wchar_t> wstr(gb2312.size());
  wchar_t* wstr_end = nullptr;
  const char* gb_end = nullptr;
  mbstate_t state{};
  int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t>>(loc).in(
      state, gb2312.data(), gb2312.data() + gb2312.size(), gb_end, wstr.data(),
      wstr.data() + wstr.size(), wstr_end);

  if (res == std::codecvt_base::ok) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
    return cutf8.to_bytes(std::wstring(wstr.data(), wstr_end));
  }

  return "";
}

template <class PriceType>
inline PriceType adjust_price(PriceType price) {
  PriceType ret = price;
  if (price >= std::numeric_limits<PriceType>::max() - PriceType(1e-6)) ret = PriceType(0);
  return ret;
}

inline char order_type(OrderType type) {
  static const char ft2ctp[] = {
      0,
      THOST_FTDC_OPT_AnyPrice,    // MARKET
      THOST_FTDC_OPT_LimitPrice,  // LIMIT
      THOST_FTDC_OPT_BestPrice,   // BEST
      THOST_FTDC_OPT_LimitPrice,  // LIMIT
      THOST_FTDC_OPT_LimitPrice   // LIMIT
  };

  return ft2ctp[static_cast<uint8_t>(type)];
}

inline Direction direction(char ctp_type) { return static_cast<Direction>(ctp_type - '0' + 1); }

inline char direction(Direction type) { return static_cast<uint8_t>(type) + '0' - 1; }

inline Offset offset(char ctp_type) {
  static const Offset ctp2ft[] = {Offset::kOpen, Offset::kClose, Offset::kUnknown,
                                  Offset::kCloseToday, Offset::kCloseYesterday};

  return ctp2ft[ctp_type - '0'];
}

inline char offset(Offset offset) {
  static const uint32_t ft2ctp[] = {
      0, THOST_FTDC_OF_Open,          THOST_FTDC_OF_Close, 0, THOST_FTDC_OF_CloseToday, 0, 0,
      0, THOST_FTDC_OF_CloseYesterday};
  return ft2ctp[static_cast<uint8_t>(offset)];
}

inline ProductType product_type(char ctp_type) {
  static const ProductType ctp2ft[] = {ProductType::kFutures, ProductType::kOptions};

  return ctp2ft[ctp_type - '1'];
}

inline char product_type(ProductType type) {
  return type == ProductType::kFutures ? THOST_FTDC_PC_Futures : THOST_FTDC_PC_Options;
}

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_COMMON_H_
