// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_GATEWAY_CTP_CTPCOMMON_H_
#define FT_GATEWAY_CTP_CTPCOMMON_H_

#include <ThostFtdcUserApiDataType.h>

#include <codecvt>
#include <limits>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include "Core/Account.h"
#include "Core/Constants.h"
#include "Core/Contract.h"
#include "Core/ContractTable.h"
#include "Core/Position.h"

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
  if (price >= std::numeric_limits<PriceType>::max() - PriceType(1e-6))
    ret = PriceType(0);
  return ret;
}

inline uint32_t order_type(char ctp_type) {
  static std::map<char, uint32_t> ctp2ft = {
      {THOST_FTDC_OPT_AnyPrice, OrderType::MARKET},
      {THOST_FTDC_OPT_LimitPrice, OrderType::LIMIT},
      {THOST_FTDC_OPT_BestPrice, OrderType::BEST}};

  return ctp2ft[ctp_type];
}

inline char order_type(uint32_t type) {
  static std::map<uint32_t, char> ft2ctp = {
      {OrderType::MARKET, THOST_FTDC_OPT_AnyPrice},
      {OrderType::FAK, THOST_FTDC_OPT_LimitPrice},
      {OrderType::FOK, THOST_FTDC_OPT_LimitPrice},
      {OrderType::LIMIT, THOST_FTDC_OPT_LimitPrice},
      {OrderType::BEST, THOST_FTDC_OPT_BestPrice}};

  return ft2ctp[type];
}

inline uint32_t direction(char ctp_type) {
  static std::map<char, uint32_t> ctp2ft = {
      {THOST_FTDC_D_Buy, Direction::BUY},
      {THOST_FTDC_D_Sell, Direction::SELL},
      {THOST_FTDC_PD_Long, Direction::BUY},
      {THOST_FTDC_PD_Short, Direction::SELL}};

  return ctp2ft[ctp_type];
}

inline char direction(uint32_t type) {
  static std::map<uint32_t, char> ft2ctp = {
      {Direction::BUY, THOST_FTDC_D_Buy}, {Direction::SELL, THOST_FTDC_D_Sell}};

  return ft2ctp[type];
}

inline uint32_t offset(char ctp_type) {
  static std::map<char, uint32_t> ctp2ft = {
      {THOST_FTDC_OF_Open, Offset::OPEN},
      {THOST_FTDC_OF_Close, Offset::CLOSE},
      {THOST_FTDC_OF_CloseToday, Offset::CLOSE_TODAY},
      {THOST_FTDC_OF_CloseYesterday, Offset::CLOSE_YESTERDAY}};
  return ctp2ft[ctp_type];
}

inline char offset(uint32_t type) {
  static std::map<uint32_t, char> ft2ctp = {
      {Offset::OPEN, THOST_FTDC_OF_Open},
      {Offset::CLOSE, THOST_FTDC_OF_Close},
      {Offset::CLOSE_TODAY, THOST_FTDC_OF_CloseToday},
      {Offset::CLOSE_YESTERDAY, THOST_FTDC_OF_CloseYesterday}};
  return ft2ctp[type];
}

inline ProductType product_type(char ctp_type) {
  static std::map<char, ProductType> ctp2ft = {
      {THOST_FTDC_PC_Futures, ProductType::FUTURES},
      {THOST_FTDC_PC_Options, ProductType::OPTIONS}};

  return ctp2ft[ctp_type];
}

inline char product_type(ProductType type) {
  static std::map<ProductType, char> ft2ctp = {
      {ProductType::FUTURES, THOST_FTDC_PC_Futures},
      {ProductType::OPTIONS, THOST_FTDC_PC_Options}};

  return ft2ctp[type];
}

}  // namespace ft

#endif  // FT_GATEWAY_CTP_CTPCOMMON_H_
