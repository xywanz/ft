// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_UTILS_CONTRACT_TABLE_H_
#define FT_SRC_UTILS_CONTRACT_TABLE_H_

#include <fmt/format.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "protocol/data_types.h"
#include "utils/string_utils.h"

namespace ft {

bool LoadContractList(const std::string& file, std::vector<Contract>* contracts);
void StoreContractList(const std::string& file, const std::vector<Contract>& contracts);

class ContractTable {
 public:
  static bool Init(std::vector<Contract>&& vec);
  static bool Init(const std::string& file);
  static void Store(const std::string& file);
  static bool is_inited() { return get()->is_inited_; }
  static std::size_t size() { return get()->contracts_.size(); }

  static const Contract* get_by_ticker(const std::string& ticker) {
    auto& ticker2contract = get()->ticker2contract_;
    auto iter = ticker2contract.find(ticker);
    if (iter == ticker2contract.end()) return nullptr;
    return iter->second;
  }

  static const Contract* get_by_index(uint32_t ticker_id) {
    auto& contracts = get()->contracts_;
    if (ticker_id == 0 || ticker_id > contracts.size()) return nullptr;
    return &contracts[ticker_id - 1];
  }

 private:
  static ContractTable* get() {
    static ContractTable ct;
    return &ct;
  }

 private:
  bool is_inited_ = false;
  std::vector<Contract> contracts_;
  std::unordered_map<std::string, Contract*> ticker2contract_;
};

}  // namespace ft

#endif  // FT_SRC_UTILS_CONTRACT_TABLE_H_
