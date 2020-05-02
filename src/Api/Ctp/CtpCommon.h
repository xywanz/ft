// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_API_CTP_CTPCOMMON_H_
#define FT_SRC_API_CTP_CTPCOMMON_H_

#include <codecvt>
#include <locale>
#include <map>
#include <string>
#include <vector>

#include <ThostFtdcUserApiDataType.h>

#include "Base/DataStruct.h"

namespace ft {

struct CtpApiDeleter {
  template<class T>
  void operator()(T* p) {
    p->RegisterSpi(nullptr);
    p->Release();
  }
};

inline bool is_error_rsp(CThostFtdcRspInfoField *rsp_info) {
  if (rsp_info && rsp_info->ErrorID != 0)
    return true;
  return false;
}

inline std::string gb2312_to_utf8(const std::string &gb2312) {
  static const std::locale loc("zh_CN.GB18030");

  std::vector<wchar_t> wstr(gb2312.size());
  wchar_t* wstr_end = nullptr;
  const char* gb_end = nullptr;
  mbstate_t state {};
  int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t>>(loc).in(
          state,
          gb2312.data(), gb2312.data() + gb2312.size(), gb_end,
          wstr.data(), wstr.data() + wstr.size(), wstr_end);

  if (res == std::codecvt_base::ok) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
      return cutf8.to_bytes(std::wstring(wstr.data(), wstr_end));
  }

  return "";
}

inline OrderType order_type(char ctp_type) {
  static std::map<char, OrderType> ctp2ft = {
    {THOST_FTDC_OPT_AnyPrice,   OrderType::MARKET},
    {THOST_FTDC_OPT_LimitPrice, OrderType::LIMIT},
    {THOST_FTDC_OPT_BestPrice,  OrderType::BEST}
  };

  return ctp2ft[ctp_type];
}

inline char order_type(OrderType type) {
  static std::map<OrderType, char> ft2ctp = {
    {OrderType::MARKET,   THOST_FTDC_OPT_AnyPrice},
    {OrderType::FAK,      THOST_FTDC_OPT_LimitPrice},
    {OrderType::FOK,      THOST_FTDC_OPT_LimitPrice},
    {OrderType::LIMIT,    THOST_FTDC_OPT_LimitPrice},
    {OrderType::BEST,     THOST_FTDC_OPT_BestPrice}
  };

  return ft2ctp[type];
}

inline Direction direction(char ctp_type) {
  static std::map<char, Direction> ctp2ft = {
    {THOST_FTDC_D_Buy,  Direction::BUY},
    {THOST_FTDC_D_Sell, Direction::SELL},
    {THOST_FTDC_PD_Long,  Direction::BUY},
    {THOST_FTDC_PD_Short, Direction::SELL}
  };

  return ctp2ft[ctp_type];
}

inline char direction(Direction type) {
  static std::map<Direction, char> ft2ctp = {
    {Direction::BUY,  THOST_FTDC_D_Buy},
    {Direction::SELL, THOST_FTDC_D_Sell}
  };

  return ft2ctp[type];
}

inline Offset offset(char ctp_type) {
  static std::map<char, Offset> ctp2ft = {
    {THOST_FTDC_OF_Open,            Offset::OPEN},
    {THOST_FTDC_OF_Close,           Offset::CLOSE},
    {THOST_FTDC_OF_ForceClose,      Offset::FORCE_CLOSE},
    {THOST_FTDC_OF_CloseToday,      Offset::CLOSE_TODAY},
    {THOST_FTDC_OF_CloseYesterday,  Offset::CLOSE_YESTERDAY},
    {THOST_FTDC_OF_ForceOff,        Offset::FORCE_OFF},
    {THOST_FTDC_OF_LocalForceClose, Offset::LOCAL_FORCE_CLOSE}
  };

  return ctp2ft[ctp_type];
}

inline char offset(Offset type) {
  static std::map<Offset, char> ft2ctp = {
    {Offset::OPEN,              THOST_FTDC_OF_Open},
    {Offset::CLOSE,             THOST_FTDC_OF_Close},
    {Offset::FORCE_CLOSE,       THOST_FTDC_OF_ForceClose},
    {Offset::CLOSE_TODAY,       THOST_FTDC_OF_CloseToday},
    {Offset::CLOSE_YESTERDAY,   THOST_FTDC_OF_CloseYesterday},
    {Offset::FORCE_OFF,         THOST_FTDC_OF_ForceOff},
    {Offset::LOCAL_FORCE_CLOSE, THOST_FTDC_OF_LocalForceClose}
  };

  return ft2ctp[type];
}

inline ProductType product_type(char ctp_type) {
  static std::map<char, ProductType> ctp2ft = {
    {THOST_FTDC_PC_Futures, ProductType::FUTURES},
    {THOST_FTDC_PC_Options, ProductType::OPTIONS}
  };

  return ctp2ft[ctp_type];
}

inline char product_type(ProductType type) {
  static std::map<ProductType, char> ft2ctp = {
    {ProductType::FUTURES, THOST_FTDC_PC_Futures},
    {ProductType::OPTIONS, THOST_FTDC_PC_Options}
  };

  return ft2ctp[type];
}

inline OrderStatus order_status(char ctp_type) {
  static std::map<char, OrderStatus> ctp2ft = {
    {THOST_FTDC_OST_Unknown,               OrderStatus::SUBMITTING},
    {THOST_FTDC_OST_NoTradeNotQueueing,    OrderStatus::SUBMITTING},
    {THOST_FTDC_OST_NoTradeQueueing,       OrderStatus::NO_TRADED},
    {THOST_FTDC_OST_PartTradedQueueing,    OrderStatus::PART_TRADED},
    {THOST_FTDC_OST_AllTraded,             OrderStatus::ALL_TRADED},
    {THOST_FTDC_OST_PartTradedNotQueueing, OrderStatus::CANCELED},
    {THOST_FTDC_OST_Canceled,              OrderStatus::CANCELED}
  };

  return ctp2ft[ctp_type];
}

}  // namespace ft

#endif  // FT_SRC_API_CTP_CTPCOMMON_H_
