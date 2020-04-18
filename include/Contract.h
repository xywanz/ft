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

inline void load_contracts(const std::string& file,
                           std::map<std::string, Contract>* contracts) {
  std::ifstream ifs(file);
  std::string line;
  std::vector<std::string> fields;
  Contract contract;

  while (std::getline(ifs, line)) {
    fields.clear();
    split(line, ",", fields);
    if (fields.empty() || fields[0].front() == '#' || fields[0].front() == '\n')
      continue;
    assert(fields.size() == 5);

    if (contracts->find(fields[0]) != contracts->end())
      continue;

    contract.ticker = std::move(fields[0]);
    ticker_split(contract.ticker, &contract.symbol, &contract.exchange);
    contract.name = std::move(fields[1]);
    contract.product_type = string2product(fields[2]);
    contract.size = std::stoi(fields[3]);
    contract.price_tick = std::stod(fields[4]);
    contracts->emplace(contract.ticker, std::move(contract));
  }
}

inline void store_contracts(const std::string& file,
                            const std::map<std::string, Contract>& contracts) {
  std::ofstream ofs(file, std::ios_base::trunc);
  std::string line = fmt::format("#ticker,name,product_type,size,price_tick\n");
  ofs << line;
  for (const auto& [ticker, contract] : contracts) {
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
      load_contracts(file, &contracts);
      is_inited = true;
    }
  }

  static const Contract* get(const std::string& ticker) {
    auto iter = contracts.find(ticker);
    if (iter == contracts.end())
      return nullptr;
    return &iter->second;
  }

 private:
  inline static std::map<std::string, Contract> contracts;
};

}  // namespace ft

#endif  // FT_INCLUDE_CONTRACT_H_
