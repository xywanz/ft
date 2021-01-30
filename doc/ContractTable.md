# ContractTable
## 概述
ContractTable类用于存储所有交易中要用到的合约信息，供交易服务端、策略程序等使用。简单来说可以把ContractTable看作一张表，表中的每一条记录代表一个合约，其结构如下所示
|ticker_id|ticker|exchange|name|price_tick|...|
|---|---|---|---|---|---|
|0|CF109|CZCE|棉花109|5.0|...|
|1|TA109|CZCE|化纤109|2.0|...|
|2|rb2111|SHFE|螺纹钢2111|1.0|...|
|...|...|...|...|...|...|
随着版本的更新，这些字段可能会改变

主要字段说明
* ticker_id：ft交易系统中ticker唯一的表示方法，整数类型。使用数值类型来表示ticker可以方便快速地进行传参、以及在不同模块间通过协议进行传递，例如发单程序通过指定发单协议中的ticker_id，来告诉server需要交易哪个ticker。同时，通过ticker_id可以快速地索引到合约具体信息。
* ticker：合约代码，gateway下单时要可能需要设置ticker字段
* exchange：交易所，gateway下单时可能要设置exchange字段

## 初始化
ContractTable为单例类，在程序启动时需要调用Init进行初始化，只能被初始化一次，全局维护一份。

通常是从合约csv文件加载，后面会提到如何生成contracts.csv
```cpp
 if (!ContractTable::Init("./contracts.csv")) {
   // handler error
 }
```
或者可以手动生成一个std::vector<Contract>，然后对其进行加载
```cpp
 std::vector<Contract> contracts;
 contracts.emplace_back({/*your contract*/});
 // ...
 if (!ContractTable::Init(std::move(contracts))) {
   // handler error
 }
```

## 生成合约csv文件
使用contract-collect工具生成合约csv文件，contract会从交易API中读取合约列表然后保存到本地。
```bash
./contract-collector --config=./my_ctp_config.yml
```

## ContracTable的使用
1. 通过ticker_id查找合约信息
```cpp
  uint32_t ticker_id = 2;
  auto* contract = ContractTable::get_by_index(ticker_id);
  std::cout << contract->exchange << std::endl;
```

2. 通过ticker查找合约信息
```cpp
  std:string ticker = "rb2111";
  auto* contract = ContractTable::get_by_ticker(ticker);
  std::cout << contract->exchange << std::endl;
```
