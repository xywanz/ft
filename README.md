# A Fast Trading System
[toc]
## 1. Introduction
### 1.1 What is ft?
ft is a fast trading system for algo trading based on C++, which focuses on building efficient strategy conveniently.

### 1.2 How to use?
```c++
// MyStrategy.cpp

#include <AlgoTrade/Strategy.h>

class MyStrategy : public ft::Strategy {
 public:
  bool on_init(AlgoTradingContext* ctx) override {
     // called when strategy mounted
     // return true if done
     // return false if you want to init your strategy later for some resources not loaded yet
  }

  void on_tick(AlgoTradingContext* ctx) override {
    // called when tick data arrives
  }

  void on_exit(AlgoTradingContext* ctx) override {
    // called when strategy unmounted
  }
};
```

### 1.3 How to build?
Use cmake to compile your strategy to dynamic library.
```cmake
add_library(MyStrategy SHARED MyStrategy.cpp)
target_link_libraries(MyStrategy ft)
```

### 1.4 Go for it
```
./strategy_loader -l libMyStrategy.so
```

### 1.5 Use it in other ways
Create a StrategyEngine and called StrategyEngine::mount to mount your strategy. It's suggest to compile your strategies to dynamic libraries, and implement a framework using StrategyEngine, then load them dynamicly. If so, you can focus on making your strategies.

### 1.6 Configurations
```yml
api: ctp  # api name.
front_addr: tcp://180.168.146.187:10130
md_server_addr: tcp://180.168.146.187:10131
broker_id: 9999
investor_id: 123456
passwd: 12345678
auth_code: 0000000000000000
app_id: simnow_client_test
ticker: rb2009.SHFE,rb2005.SHFE  # subscribed list (for market data).
```

### 1.7 How to build and run examples
```bash
### build
cd ft  # enter root directory of the project ft
mkdir build && cd build
cmake .. && make

### run
./contract_collector  # build contract table first. Must! After that, you can see contract.csv in ../config/
vim ../config/login.yml  # config your broker id, investor id and password
./strategy_loader -l libgrid_strategy.so # run demo of grid strategy
```

## 2. ...
