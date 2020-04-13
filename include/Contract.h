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

inline void load_contract_file(const std::string& file,
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
    assert(fields.size() == 3);

    if (contracts->find(fields[0]) != contracts->end())
      continue;

    contract.ticker = std::move(fields[0]);
    ticker_split(contract.ticker, &contract.symbol, &contract.exchange);
    contract.size = std::stoi(fields[1]);
    contract.price_tick = std::stod(fields[2]);
    contracts->emplace(contract.ticker, std::move(contract));
  }
}

class ContractTable {
 public:
  static bool init(const std::string& file) {
    static bool is_inited;

    if (!is_inited) {
      load_contract_file(file, &contracts);
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
