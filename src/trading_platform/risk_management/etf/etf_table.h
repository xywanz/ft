// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_RISK_MANAGEMENT_ETF_ETF_TABLE_H_
#define FT_SRC_RISK_MANAGEMENT_ETF_ETF_TABLE_H_

#include <spdlog/spdlog.h>

#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "core/contract.h"
#include "core/contract_table.h"
#include "risk_management/etf/etf.h"
#include "utils/string_utils.h"

namespace ft {

class EtfTable {
 public:
  static bool init(const std::string& etf_list,
                   const std::string& etf_components) {
    std::ifstream ifs(etf_list);
    if (!ifs) {
      spdlog::error("etf list file not found: {}", etf_list);
      return false;
    }

    etf_vec_.resize(ContractTable::size() + 1);

    std::string line;
    std::vector<std::string> tokens;
    std::getline(ifs, line);  // skip head
    while (std::getline(ifs, line)) {
      tokens.clear();
      split(line, ",", &tokens);

      auto contract = ContractTable::get_by_ticker(tokens[0]);
      if (!contract) continue;

      auto etf = new ETF{};
      etf->contract = contract;
      etf->unit = std::stoi(tokens[2]);
      etf->purchase_allowed = std::stoi(tokens[3]);
      etf->redeem_allowed = std::stoi(tokens[4]);
      etf->max_cash_ratio = std::stod(tokens[5]);
      etf->cash_component = std::stod(tokens[6]);
      etf->must_cash_substitution = 0;

      etf_vec_[contract->index] = etf;
      etf_map_[contract->ticker] = etf;
    }

    ifs.close();
    ifs.open(etf_components);
    if (!ifs) {
      spdlog::error("etf components file not found: {}", etf_components);
      return false;
    }

    std::getline(ifs, line);  // skip head
    while (std::getline(ifs, line)) {
      tokens.clear();
      split(line, ",", &tokens);

      auto iter = etf_map_.find(tokens[0]);
      if (iter == etf_map_.end()) continue;

      auto etf = iter->second;

      int replace_type = std::stoul(tokens[4]);
      if (replace_type == MUST || replace_type == RECOMPUTE) {
        etf->must_cash_substitution += std::stod(tokens[6]);
      } else {
        auto contract = ContractTable::get_by_ticker(tokens[1]);
        // 查找不到成分股信息，直接禁止该ETF的申赎
        if (!contract) {
          spdlog::warn("[etf] {} is not allowed to purchase/redeem",
                       etf->contract->ticker);
          etf_vec_[etf->contract->index] = nullptr;
          etf_map_.erase(iter);
          delete etf;
          continue;
        }

        ComponentStock component{};
        component.contract = contract;
        component.etf_contract = etf->contract;
        component.replace_type = replace_type;
        component.volume = std::stod(tokens[5]);
        etf->components.emplace(contract->index, component);
      }
    }

    return true;
  }

  static const ETF* get_by_index(uint32_t ticker_index) {
    if (ticker_index >= etf_vec_.size()) return nullptr;
    return etf_vec_[ticker_index];
  }

  static const ETF* get_by_ticker(const std::string& ticker) {
    auto iter = etf_map_.find(ticker);
    if (iter == etf_map_.end()) return nullptr;
    return iter->second;
  }

 private:
  static inline std::vector<ETF*> etf_vec_;
  static inline std::map<std::string, ETF*> etf_map_;
};

}  // namespace ft

#endif  // FT_SRC_RISK_MANAGEMENT_ETF_ETF_TABLE_H_
