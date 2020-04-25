// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CONTRACT_H_
#define FT_INCLUDE_CONTRACT_H_

#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <cppex/string.h>
#include <fmt/format.h>

#include "Common.h"

namespace ft {

struct Contract {
  std::string   symbol;
  std::string   exchange;
  std::string   ticker;
  std::string   name;
  ProductType   product_type;
  int           size;
  double        price_tick;
};

inline bool load_contracts(const std::string& file, std::vector<Contract>* contracts) {
  std::ifstream ifs(file);
  std::string line;
  std::vector<std::string> fields;
  Contract contract;

  if (!ifs)
    return false;

  std::getline(ifs, line);  // skip header
  while (std::getline(ifs, line)) {
    fields.clear();
    split(line, ",", fields);
    if (fields.empty() || fields[0].front() == '\n')
      continue;

    if (fields.size() != 5)
      return false;

    contract.ticker = std::move(fields[0]);
    ticker_split(contract.ticker, &contract.symbol, &contract.exchange);
    contract.name = std::move(fields[1]);
    contract.product_type = string2product(fields[2]);
    contract.size = std::stoi(fields[3]);
    contract.price_tick = std::stod(fields[4]);
    contracts->emplace_back(std::move(contract));
  }

  return true;
}

inline void store_contracts(const std::string& file,
                            const std::vector<Contract>& contracts) {
  std::ofstream ofs(file, std::ios_base::trunc);
  std::string line = fmt::format("ticker,name,product_type,size,price_tick\n");
  ofs << line;
  for (const auto& contract : contracts) {
    line = fmt::format("{},{},{},{},{}\n",
                       contract.ticker,
                       contract.name,
                       to_string(contract.product_type),
                       contract.size,
                       contract.price_tick);
    ofs << line;
  }

  ofs.close();
}

class ContractTable {
 public:
  static bool init(const std::string& file) {
    static bool is_inited = false;

    if (!is_inited) {
      if (!load_contracts(file, &contracts))
        return false;

      for (auto& contract : contracts) {
        ticker2contract.emplace(contract.ticker, &contract);
        symbol2contract.emplace(contract.symbol, &contract);
      }

      is_inited = true;
    }

    return true;
  }

  static const Contract* get_by_ticker(const std::string& ticker) {
    auto iter = ticker2contract.find(ticker);
    if (iter == ticker2contract.end())
      return nullptr;
    return iter->second;
  }

  static const Contract* get_by_symbol(const std::string& symbol) {
    auto iter = symbol2contract.find(symbol);
    if (iter == symbol2contract.end())
      return nullptr;
    return iter->second;
  }

 private:
  inline static std::vector<Contract> contracts;
  inline static std::map<std::string, Contract*> ticker2contract;
  inline static std::map<std::string, Contract*> symbol2contract;
};

}  // namespace ft

#endif  // FT_INCLUDE_CONTRACT_H_
