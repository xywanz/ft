// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CONTRACTTABLE_H_
#define FT_INCLUDE_CONTRACTTABLE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Core/Constants.h"
#include "Core/Contract.h"

namespace ft {

inline bool load_contracts(const std::string& file,
                           std::vector<Contract>* contracts) {
  std::ifstream ifs(file);
  std::string line;
  std::vector<std::string> fields;
  Contract contract;

  if (!ifs) return false;

  std::getline(ifs, line);  // skip header
  while (std::getline(ifs, line)) {
    fields.clear();
    split(line, ",", fields);
    if (fields.empty() || fields[0].front() == '\n') continue;

    if (fields.size() != 11) return false;

    contract.ticker = std::move(fields[0]);
    ticker_split(contract.ticker, &contract.symbol, &contract.exchange);
    contract.name = std::move(fields[1]);
    contract.product_type = string2product(fields[2]);
    contract.size = std::stoi(fields[3]);
    contract.price_tick = std::stod(fields[4]);
    contract.max_market_order_volume = std::stoi(fields[5]);
    contract.min_market_order_volume = std::stoi(fields[6]);
    contract.max_limit_order_volume = std::stoi(fields[7]);
    contract.min_limit_order_volume = std::stoi(fields[8]);
    contract.delivery_year = std::stoi(fields[9]);
    contract.delivery_month = std::stoi(fields[10]);
    contracts->emplace_back(std::move(contract));
  }

  return true;
}

inline void store_contracts(const std::string& file,
                            const std::vector<Contract>& contracts) {
  std::ofstream ofs(file, std::ios_base::trunc);
  std::string line = fmt::format(
      "ticker,name,product_type,size,price_tick,max_market_order_volume,"
      "min_market_order_volume,max_limit_order_volume,min_limit_order_volume,"
      "delivery_year,delivery_month\n");
  ofs << line;
  for (const auto& contract : contracts) {
    line = fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{}\n", contract.ticker, contract.name,
        to_string(contract.product_type), contract.size, contract.price_tick,
        contract.max_market_order_volume, contract.min_market_order_volume,
        contract.max_limit_order_volume, contract.min_limit_order_volume,
        contract.delivery_year, contract.delivery_month);
    ofs << line;
  }

  ofs.close();
}

class ContractTable {
 public:
  static bool init(const std::string& file) {
    static bool is_inited = false;

    if (!is_inited) {
      if (!load_contracts(file, &contracts)) return false;

      for (std::size_t i = 0; i < contracts.size(); ++i) {
        auto& contract = contracts[i];
        contract.index = i + 1;
        ticker2contract.emplace(contract.ticker, &contract);
        symbol2contract.emplace(contract.symbol, &contract);
      }

      is_inited = true;
    }

    return true;
  }

  static const Contract* get_by_ticker(const std::string& ticker) {
    auto iter = ticker2contract.find(ticker);
    if (iter == ticker2contract.end()) return nullptr;
    return iter->second;
  }

  static const Contract* get_by_symbol(const std::string& symbol) {
    auto iter = symbol2contract.find(symbol);
    if (iter == symbol2contract.end()) return nullptr;
    return iter->second;
  }

  static const Contract* get_by_index(uint64_t ticker_index) {
    if (ticker_index == 0 || ticker_index > contracts.size()) return nullptr;
    return &contracts[ticker_index - 1];
  }

 private:
  inline static std::vector<Contract> contracts;
  inline static std::map<std::string, Contract*> ticker2contract;
  inline static std::map<std::string, Contract*> symbol2contract;
};

}  // namespace ft

#endif  // FT_INCLUDE_CONTRACTTABLE_H_
