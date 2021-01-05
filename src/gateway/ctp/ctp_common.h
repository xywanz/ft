// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_CTP_CTP_COMMON_H_
#define FT_SRC_GATEWAY_CTP_CTP_COMMON_H_

#include <ThostFtdcUserApiDataType.h>

#include <codecvt>
#include <limits>
#include <locale>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "trading_server/datastruct/account.h"
#include "trading_server/datastruct/constants.h"
#include "trading_server/datastruct/contract.h"
#include "trading_server/datastruct/contract_table.h"
#include "trading_server/datastruct/position.h"

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

inline char order_type(uint32_t type) {
  static const char ft2ctp[] = {
      0,
      THOST_FTDC_OPT_AnyPrice,    // MARKET
      THOST_FTDC_OPT_LimitPrice,  // LIMIT
      THOST_FTDC_OPT_BestPrice,   // BEST
      THOST_FTDC_OPT_LimitPrice,  // LIMIT
      THOST_FTDC_OPT_LimitPrice   // LIMIT
  };

  return ft2ctp[type];
}

inline uint32_t direction(char ctp_type) { return ctp_type - '0' + 1; }

inline char direction(uint32_t type) { return type + '0' - 1; }

inline uint32_t offset(char ctp_type) {
  static const uint32_t ctp2ft[] = {Offset::OPEN, Offset::CLOSE, 0, Offset::CLOSE_TODAY,
                                    Offset::CLOSE_YESTERDAY};

  return ctp2ft[ctp_type - '0'];
}

inline char offset(uint32_t type) {
  static const uint32_t ft2ctp[] = {
      0, THOST_FTDC_OF_Open,          THOST_FTDC_OF_Close, 0, THOST_FTDC_OF_CloseToday, 0, 0,
      0, THOST_FTDC_OF_CloseYesterday};
  return ft2ctp[type];
}

inline ProductType product_type(char ctp_type) {
  static const ProductType ctp2ft[] = {ProductType::FUTURES, ProductType::OPTIONS};

  return ctp2ft[ctp_type - '1'];
}

inline char product_type(ProductType type) {
  return type == ProductType::FUTURES ? THOST_FTDC_PC_Futures : THOST_FTDC_PC_Options;
}

}  // namespace ft

#endif  // FT_SRC_GATEWAY_CTP_CTP_COMMON_H_
