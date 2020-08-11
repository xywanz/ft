// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_CEP_OMS_OMS_H_
#define FT_SRC_CEP_OMS_OMS_H_

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "cep/data/account.h"
#include "cep/data/config.h"
#include "cep/data/error_code.h"
#include "cep/data/md_snapshot.h"
#include "cep/data/order.h"
#include "cep/interface/gateway.h"
#include "cep/interface/oms_interface.h"
#include "cep/oms/portfolio.h"
#include "cep/rms/rms.h"
#include "cep/rms/types.h"
#include "ipc/redis_md_helper.h"

namespace ft {

class OMS : public OMSInterface {
 public:
  OMS();
  ~OMS();

  bool login(const Config& config);

  void process_cmd();

  void close();

  static uint64_t version() { return 202006222311; }

 private:
  void process_cmd_from_redis();
  void process_cmd_from_queue();
  void execute_cmd(const TraderCommand& cmd);

  bool send_order(const TraderCommand& cmd);
  void cancel_order(uint64_t order_id);
  void cancel_for_ticker(uint32_t tid);
  void cancel_all();

  void on_tick(TickData* tick) override;

  void on_order_accepted(OrderAcceptance* rsp) override;
  void on_order_rejected(OrderRejection* rsp) override;
  void on_order_traded(Trade* rsp) override;
  void on_order_canceled(OrderCancellation* rsp) override;
  void on_order_cancel_rejected(OrderCancelRejection* rsp) override;

 private:
  void handle_account(Account* account);
  void handle_positions(std::vector<Position>* positions);
  void handle_trades(std::vector<Trade>* trades);
  void handle_timer();

  void on_primary_market_traded(Trade* rsp);    // ETF申赎
  void on_secondary_market_traded(Trade* rsp);  // 二级市场买卖

  uint64_t next_order_id() { return next_oms_order_id_++; }

 private:
  std::unique_ptr<Gateway> gateway_{nullptr};

  volatile bool is_logon_{false};
  uint64_t next_oms_order_id_{1};

  Account account_;
  Portfolio portfolio_;
  OrderMap order_map_;
  std::unique_ptr<RMS> rms_{nullptr};
  RedisMdPusher md_pusher_;
  MdSnapshot md_snapshot_;
  std::mutex mutex_;
  std::unique_ptr<std::thread> timer_thread_;

  int cmd_queue_key_ = 0;
};

}  // namespace ft

#endif  // FT_SRC_CEP_OMS_OMS_H_
