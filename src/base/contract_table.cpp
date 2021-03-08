// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/base/contract_table.h"

#include "fmt/format.h"
#include "ft/utils/protocol_utils.h"
#include "ft/utils/string_utils.h"

namespace ft {

bool LoadContractList(const std::string& file, std::vector<Contract>* contracts) {
  std::ifstream ifs(file);
  std::string line;
  std::vector<std::string> fields;
  Contract contract;

  if (!ifs) return false;

  std::getline(ifs, line);  // skip header
  while (std::getline(ifs, line)) {
    fields.clear();
    StringSplit(line, ",", &fields);
    if (fields.empty() || fields[0].front() == '\n') continue;

    if (fields.size() != 14) return false;

    std::size_t index = 0;
    contract.ticker = std::move(fields[index++]);
    contract.exchange = std::move(fields[index++]);
    contract.name = std::move(fields[index++]);
    contract.product_type = StringToProductType(fields[index++]);
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

void StoreContractList(const std::string& file, const std::vector<Contract>& contracts) {
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
    line = fmt::format(
        "{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n", contract.ticker, contract.exchange,
        contract.name, ToString(contract.product_type), contract.size, contract.price_tick,
        contract.long_margin_rate, contract.short_margin_rate, contract.max_market_order_volume,
        contract.min_market_order_volume, contract.max_limit_order_volume,
        contract.min_limit_order_volume, contract.delivery_year, contract.delivery_month);

    ofs << line;
  }

  ofs.close();
}

ContractTable* ContractTable::get() {
  static ContractTable ct;
  return &ct;
}

bool ContractTable::Init(std::vector<Contract>&& vec) {
  if (!get()->is_inited_) {
    auto& contracts = get()->contracts_;
    auto& ticker2contract = get()->ticker2contract_;
    contracts = std::move(vec);
    for (std::size_t i = 0; i < contracts.size(); ++i) {
      auto& contract = contracts[i];
      contract.ticker_id = i + 1;
      ticker2contract.emplace(contract.ticker, &contract);
    }
    get()->is_inited_ = true;
  }

  return true;
}

bool ContractTable::Init(const std::string& file) {
  if (!get()->is_inited_) {
    std::vector<Contract> contracts;
    if (!LoadContractList(file, &contracts)) {
      return false;
    }
    return ContractTable::Init(std::move(contracts));
  }

  return true;
}

void ContractTable::Store(const std::string& file) { StoreContractList(file, get()->contracts_); }

}  // namespace ft
