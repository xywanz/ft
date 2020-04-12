// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_FIELDMAPPER_H_
#define FT_INCLUDE_CTP_FIELDMAPPER_H_

#include <map>

#include <ThostFtdcUserApiDataType.h>

#include "Order.h"

namespace ft {

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

#endif  // FT_INCLUDE_CTP_FIELDMAPPER_H_
