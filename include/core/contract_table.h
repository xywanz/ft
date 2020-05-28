// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CORE_CONTRACT_TABLE_H_
#define FT_INCLUDE_CORE_CONTRACT_TABLE_H_

#include <fmt/format.h>

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/constants.h"
#include "core/contract.h"
#include "utils/string_utils.h"

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
    split(line, ",", &fields);
    if (fields.empty() || fields[0].front() == '\n') continue;

    if (fields.size() != 14) return false;

    std::size_t index = 0;
    contract.ticker = std::move(fields[index++]);
    contract.exchange = std::move(fields[index++]);
    contract.name = std::move(fields[index++]);
    contract.product_type = string2product(fields[index++]);
    contract.size = std::stoi(fields[index++]);
    contract.price_tick = std::stod(fields[index++]);
    contract.long_margin_rate = std::stod(fields[index++]);
    contract.short_margin_rate = std::stod(fields[index++]);
    contract.max_market_order_volume = std::stoi(fields[index++]);
    contract.min_market_order_volume = std::stoi(fields[index++]);
    contract.max_limit_order_volume = std::stoi(fields[index++]);
    contract.min_limit_order_volume = std::stoi(fields[index++]);
    contract.delivery_year = std::stoi(fields[index++]);
    contract.delivery_month = std::stoi(fields[index++]);
    contracts->emplace_back(std::move(contract));
  }

  return true;
}

inline void store_contracts(const std::string& file,
                            const std::vector<Contract>& contracts) {
  std::ofstream ofs(file, std::ios_base::trunc);
  ofs << "ticker,"
         "exchange,"
         "name,"
         "product_type,"
         "size,"
         "price_tick,"
         "long_margin_rate,"
         "short_margin_rate,"
         "max_market_order_volume,"
         "min_market_order_volume,"
         "max_limit_order_volume,"
         "min_limit_order_volume,"
         "delivery_year,"
         "delivery_month\n";

  // std::stringstream ss;
  std::string line;
  for (const auto& contract : contracts) {
    // ss << contract.ticker << ',' << contract.exchange << ',' << contract.name
    //    << ',' << to_string(contract.product_type) << ',' << contract.size <<
    //    ','
    //    << contract.price_tick << ',' << contract.max_market_order_volume <<
    //    ','
    //    << contract.min_market_order_volume << ','
    //    << contract.max_limit_order_volume << ','
    //    << contract.min_limit_order_volume << ',' << contract.delivery_year
    //    << ',' << contract.delivery_month << '\n';
    line = fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n", contract.ticker,
        contract.exchange, contract.name, to_string(contract.product_type),
        contract.size, contract.price_tick, contract.long_margin_rate,
        contract.short_margin_rate, contract.max_market_order_volume,
        contract.min_market_order_volume, contract.max_limit_order_volume,
        contract.min_limit_order_volume, contract.delivery_year,
        contract.delivery_month);

    // ss >> line;
    ofs << line;
    // ss.clear();
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

  static const Contract* get_by_index(uint32_t ticker_index) {
    if (ticker_index == 0 || ticker_index > contracts.size()) return nullptr;
    return &contracts[ticker_index - 1];
  }

 private:
  inline static std::vector<Contract> contracts;
  inline static std::map<std::string, Contract*> ticker2contract;
};

}  // namespace ft

#endif  // FT_INCLUDE_CORE_CONTRACT_TABLE_H_
