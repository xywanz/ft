// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_GATEWAY_H_
#define FT_INCLUDE_GATEWAY_H_

#include <map>
#include <memory>
#include <string>

#include "Core/LoginParams.h"
#include "Core/TradingEngineInterface.h"

namespace ft {

struct OrderReq {
  uint64_t ticker_index;
  uint64_t type;
  uint64_t direction;
  uint64_t offset;
  int64_t volume = 0;
  double price = 0;
};

class Gateway {
 public:
  explicit Gateway(TradingEngineInterface* engine) : engine_(engine) {}

  virtual ~Gateway() {}

  virtual bool login(const LoginParams& params) { return false; }

  virtual void logout() {}

  virtual uint64_t send_order(const OrderReq* order) { return 0; }

  virtual bool cancel_order(uint64_t order_id) { return false; }

  virtual bool query_contract(const std::string& ticker) { return false; }

  virtual bool query_contracts() { return false; }

  virtual bool query_position(const std::string& ticker) { return false; }

  virtual bool query_positions() { return false; }

  virtual bool query_account() { return false; }

  virtual bool query_margin_rate(const std::string& ticker) { return false; }

  virtual bool query_commision_rate(const std::string& ticker) { return false; }

 protected:
  TradingEngineInterface* engine_;
};

using __GATEWAY_CREATE_FUNC = std::function<Gateway*(TradingEngineInterface*)>;
std::map<std::string, __GATEWAY_CREATE_FUNC>& __get_api_map();
Gateway* create_gateway(const std::string& name,
                        TradingEngineInterface* engine);
void destroy_gateway(Gateway* api);

#define REGISTER_GATEWAY(name, type)                    \
  static inline ::ft::Gateway* __create_##type(         \
      ::ft::TradingEngineInterface* engine) {           \
    return new type(engine);                            \
  }                                                     \
  static inline bool __is_##type##_registered = [] {    \
    auto& type_map = ::ft::__get_api_map();             \
    auto res = type_map.emplace(name, __create_##type); \
    return res.second;                                  \
  }()

}  // namespace ft

#endif  // FT_INCLUDE_GATEWAY_H_
