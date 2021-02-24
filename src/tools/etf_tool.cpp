// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <xtp_trader_api.h>

#include <fstream>

#include "trading_server/risk_management/etf/etf.h"
#include "utils/config_loader.h"
#include "utils/misc.h"

const char* XtpMaketStr(XTP_MARKET_TYPE market) { return market == XTP_MKT_SZ_A ? "SZ" : "SH"; }

ft::ReplaceType RelaceType(ETF_REPLACE_TYPE type) {
  if (type == ERT_CASH_FORBIDDEN)
    return ft::FORBIDDEN;
  else if (type == ERT_CASH_OPTIONAL)
    return ft::OPTIONAL;
  else if (type == ERT_CASH_MUST || type == ERT_CASH_MUST_INTER_OTHER ||
           type == ERT_CASH_MUST_INTER_SZ)
    return ft::MUST;
  else if (type == ERT_CASH_RECOMPUTE_INTER_OTHER || type == ERT_CASH_RECOMPUTE_INTER_SZ)
    return ft::RECOMPUTE;
  abort();
}

struct EtfInfo {
  std::string ticker;
  XTP_MARKET_TYPE market;
};

class EtfTool : public XTP::API::TraderSpi {
 public:
  bool Login(const ft::Config& config) {
    uint32_t seed = time(nullptr);
    uint8_t client_id = rand_r(&seed) & 0xff;
    trade_api_ = XTP::API::TraderApi::CreateTraderApi(client_id, ".");
    if (!trade_api_) {
      spdlog::error("[EtfTools::Login] Failed to CreateTraderApi");
      return false;
    }

    char protocol[32]{};
    char ip[32]{};
    int port = 0;

    try {
      int ret =
          sscanf(config.trade_server_address.c_str(), "%[^:]://%[^:]:%d", protocol, ip, &port);
      if (ret != 3) {
        spdlog::error("[EtfTools::Login] Invalid trade server: {}", config.trade_server_address);
        return false;
      }
    } catch (...) {
      spdlog::error("[EtfTools::Login] Invalid trade server: {}", config.trade_server_address);
      return false;
    }

    XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
    if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

    trade_api_->SubscribePublicTopic(XTP_TERT_QUICK);
    trade_api_->RegisterSpi(this);
    trade_api_->SetSoftwareKey(config.auth_code.c_str());
    sess_ =
        trade_api_->Login(ip, port, config.investor_id.c_str(), config.password.c_str(), sock_type);
    if (sess_ == 0) {
      spdlog::error("[EtfTools::Login] Failed to Call API Login: {}",
                    trade_api_->GetApiLastError()->error_msg);
      return false;
    }

    return true;
  }

  void set_output_path(const std::string& path) {
    output_path_ = path;
    if (output_path_.back() == '/') output_path_.pop_back();
  }

  bool QueryEtfList() {
    auto file = fmt::format("{}/etf_list.csv", output_path_);
    ofs_.open(file);
    if (!ofs_) {
      spdlog::error("[EtfToool::QueryEtfList] Failed to create {}", file);
      return false;
    }
    ofs_ << "ticker,market,unit,purchase_status,redeem_status,max_cash_ratio,"
            "cash_component\n";

    XTPQueryETFBaseReq req{};
    if (trade_api_->QueryETF(&req, sess_, next_req_id()) != 0) {
      spdlog::error("[XtpTradeApi::QueryTradeList] {}", trade_api_->GetApiLastError()->error_msg);
      return false;
    }

    bool ret = wait_sync();
    ofs_.close();
    return ret;
  }

  void OnQueryETF(XTPQueryETFBaseRsp* etf_info, XTPRI* error_info, int request_id, bool is_last,
                  uint64_t session_id) override {
    UNUSED(request_id);

    if (error_info && error_info->error_id != 0) {
      spdlog::error("[XtpTradeApi::OnQueryETF] {}", error_info->error_msg);
      exit(-1);
    }

    if (etf_info) {
      etf_list_.push_back({etf_info->etf, etf_info->market});
      ofs_ << fmt::format("{},{},{},{},{},{},{}\n", etf_info->etf, XtpMaketStr(etf_info->market),
                          etf_info->unit, etf_info->subscribe_status, etf_info->redemption_status,
                          etf_info->max_cash_ratio, etf_info->cash_component);
    }

    if (is_last) done();
  }

  bool QueryEtfBaskets() {
    auto file = fmt::format("{}/etf_components.csv", output_path_);
    ofs_.open(file);
    if (!ofs_) {
      spdlog::error("[EtfToool::QueryEtfBaskets] Failed to create {}", file);
      return false;
    }
    ofs_ << "etf,component,etf_market,component_market,RelaceType,quantity,"
            "amount\n";

    for (const auto& etf : etf_list_) {
      spdlog::info("export components of {} ...", etf.ticker);
      XTPQueryETFComponentReq req{};
      req.market = etf.market;
      strncpy(req.ticker, etf.ticker.c_str(), sizeof(req.ticker));
      if (trade_api_->QueryETFTickerBasket(&req, sess_, next_req_id()) != 0) {
        return false;
      }

      if (!wait_sync()) return false;
      usleep(300000);
    }

    ofs_.close();
    return true;
  }

  void OnQueryETFBasket(XTPQueryETFComponentRsp* etf_component_info, XTPRI* error_info,
                        int request_id, bool is_last, uint64_t session_id) override {
    if (error_info && error_info->error_id != 0) {
      spdlog::error("[XtpTradeApi::OnQueryETF] {}", error_info->error_msg);
      exit(-1);
    }

    if (etf_component_info) {
      ofs_ << fmt::format("{},{},{},{},{},{},{}\n", etf_component_info->ticker,
                          etf_component_info->component_ticker,
                          XtpMaketStr(etf_component_info->market),
                          XtpMaketStr(etf_component_info->component_market),
                          RelaceType(etf_component_info->replace_type),
                          etf_component_info->quantity, etf_component_info->amount);
    }

    if (is_last) done();
  }

 private:
  int next_req_id() { return next_req_id_++; }

  void done() { is_done_ = true; }

  void error() { is_error_ = true; }

  bool wait_sync() {
    while (!is_done_)
      if (is_error_) return false;

    is_done_ = false;
    return true;
  }

 private:
  XTP::API::TraderApi* trade_api_;

  uint64_t sess_;
  std::atomic<uint32_t> next_req_id_ = 1;
  volatile bool is_done_ = false;
  volatile bool is_error_ = false;
  std::string output_path_ = ".";
  std::ofstream ofs_;

  std::list<EtfInfo> etf_list_;
};

int main() {
  ft::Config config;
  ft::LoadConfig("../config/my_xtp_config.yml", &config);

  EtfTool etf_tool;
  if (!etf_tool.Login(config)) {
    spdlog::error("failed to Login");
    return -1;
  }

  if (!etf_tool.QueryEtfList()) {
    spdlog::error("failed to query etf list");
    return -1;
  }
  spdlog::info("successfully export etf list");

  if (!etf_tool.QueryEtfBaskets()) {
    spdlog::error("failed to query etf baskets");
    return -1;
  }
  spdlog::info("successfully export etf baskets");
}
