// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <xtp_trader_api.h>

#include <fstream>

#include "trading_system/config_loader.h"
#include "utils/misc.h"

class EtfTool : public XTP::API::TraderSpi {
 public:
  bool login(const ft::Config& config) {
    uint32_t seed = time(nullptr);
    uint8_t client_id = rand_r(&seed) & 0xff;
    trade_api_ = XTP::API::TraderApi::CreateTraderApi(client_id, ".");
    if (!trade_api_) {
      spdlog::error("[EtfTools::login] Failed to CreateTraderApi");
      return false;
    }

    char protocol[32]{};
    char ip[32]{};
    int port = 0;

    try {
      int ret = sscanf(config.trade_server_address.c_str(), "%[^:]://%[^:]:%d",
                       protocol, ip, &port);
      if (ret != 3) {
        spdlog::error("[EtfTools::login] Invalid trade server: {}",
                      config.trade_server_address);
        return false;
      }
    } catch (...) {
      spdlog::error("[EtfTools::login] Invalid trade server: {}",
                    config.trade_server_address);
      return false;
    }

    XTP_PROTOCOL_TYPE sock_type = XTP_PROTOCOL_TCP;
    if (strcmp(protocol, "udp") == 0) sock_type = XTP_PROTOCOL_UDP;

    trade_api_->SubscribePublicTopic(XTP_TERT_QUICK);
    trade_api_->RegisterSpi(this);
    trade_api_->SetSoftwareKey(config.auth_code.c_str());
    session_id_ = trade_api_->Login(ip, port, config.investor_id.c_str(),
                                    config.password.c_str(), sock_type);
    if (session_id_ == 0) {
      spdlog::error("[EtfTools::login] Failed to Call API login: {}",
                    trade_api_->GetApiLastError()->error_msg);
      return false;
    }

    return true;
  }

  void set_output_path(const std::string& path) {
    output_path_ = path;
    if (output_path_.back() == '/') output_path_.pop_back();
  }

  bool query_etf_list() {
    auto file = fmt::format("{}/etf_list.csv", output_path_);
    ofs_.open(file);
    if (!ofs_) {
      spdlog::error("[EtfToool::query_etf_list] Failed to create {}", file);
      return false;
    }
    ofs_ << "ticker,market,unit,purchase_status,redeem_status,max_cash_ratio,"
            "cash_component\n";

    XTPQueryETFBaseReq req{};
    if (trade_api_->QueryETF(&req, session_id_, next_req_id()) != 0) {
      spdlog::error("[XtpTradeApi::query_trades] {}",
                    trade_api_->GetApiLastError()->error_msg);
      return false;
    }

    bool ret = wait_sync();
    ofs_.close();
    return ret;
  }

  void OnQueryETF(XTPQueryETFBaseRsp* etf_info, XTPRI* error_info,
                  int request_id, bool is_last, uint64_t session_id) override {
    UNUSED(request_id);

    if (error_info && error_info->error_id != 0) {
      spdlog::error("[XtpTradeApi::OnQueryETF] {}", error_info->error_msg);
      done();
      return;
    }

    if (etf_info) {
      ofs_ << fmt::format("{},{},{},{},{},{},{}\n", etf_info->etf,
                          etf_info->market == 1 ? "SZ" : "SH", etf_info->unit,
                          etf_info->subscribe_status,
                          etf_info->redemption_status, etf_info->max_cash_ratio,
                          etf_info->cash_component);
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

  uint64_t session_id_;
  std::atomic<uint32_t> next_req_id_ = 1;
  volatile bool is_done_ = false;
  volatile bool is_error_ = false;
  std::string output_path_ = ".";
  std::ofstream ofs_;
};

int main() {
  ft::Config config;
  ft::load_config("../config/my_xtp_config.yml", &config);

  EtfTool etf_tool;
  if (!etf_tool.login(config)) {
    spdlog::error("failed to login");
    return -1;
  }
  if (!etf_tool.query_etf_list()) {
    spdlog::error("failed to query etf list");
    return -1;
  }

  spdlog::info("successfully export etf list");
}
