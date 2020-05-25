# FT 算法交易系统
| Platform | Build                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------- |
| Linux    | [![Build Status](https://travis-ci.com/DuckDuckDuck0/ft.svg?branch=master)](https://travis-ci.com/DuckDuckDuck0/ft) |

不断更新中，大家有什么想法或是发现了bug，可以一起在issue上面自由讨论，也欢迎大家加入到开发中。如有问题我将全力提供支持。  

目录
=================


- [FT 算法交易系统](#ft-%e7%ae%97%e6%b3%95%e4%ba%a4%e6%98%93%e7%b3%bb%e7%bb%9f)
- [目录](#%e7%9b%ae%e5%bd%95)
  - [1. 更新说明](#1-%e6%9b%b4%e6%96%b0%e8%af%b4%e6%98%8e)
  - [2. 运行示例](#2-%e8%bf%90%e8%a1%8c%e7%a4%ba%e4%be%8b)
  - [3. 简介](#3-%e7%ae%80%e4%bb%8b)
    - [3.1. FT 是什么](#31-ft-%e6%98%af%e4%bb%80%e4%b9%88)
    - [3.2. 基本架构](#32-%e5%9f%ba%e6%9c%ac%e6%9e%b6%e6%9e%84)
  - [4. 交易引擎模块设计](#4-%e4%ba%a4%e6%98%93%e5%bc%95%e6%93%8e%e6%a8%a1%e5%9d%97%e8%ae%be%e8%ae%a1)
    - [4.1. 模块之间的交互](#41-%e6%a8%a1%e5%9d%97%e4%b9%8b%e9%97%b4%e7%9a%84%e4%ba%a4%e4%ba%92)
    - [4.2. 交易管理模块](#42-%e4%ba%a4%e6%98%93%e7%ae%a1%e7%90%86%e6%a8%a1%e5%9d%97)
      - [4.2.1. 接口设计](#421-%e6%8e%a5%e5%8f%a3%e8%ae%be%e8%ae%a1)
      - [4.2.2. 订单转发](#422-%e8%ae%a2%e5%8d%95%e8%bd%ac%e5%8f%91)
      - [4.2.3. 仓位管理](#423-%e4%bb%93%e4%bd%8d%e7%ae%a1%e7%90%86)
      - [4.2.4. 订单管理](#424-%e8%ae%a2%e5%8d%95%e7%ae%a1%e7%90%86)
      - [4.2.5. 资金管理](#425-%e8%b5%84%e9%87%91%e7%ae%a1%e7%90%86)
    - [4.3. 风险管理模块](#43-%e9%a3%8e%e9%99%a9%e7%ae%a1%e7%90%86%e6%a8%a1%e5%9d%97)
    - [4.4. 交易网关模块](#44-%e4%ba%a4%e6%98%93%e7%bd%91%e5%85%b3%e6%a8%a1%e5%9d%97)
    - [4.5. 交易引擎初始化](#45-%e4%ba%a4%e6%98%93%e5%bc%95%e6%93%8e%e5%88%9d%e5%a7%8b%e5%8c%96)
  - [5. 策略模型](#5-%e7%ad%96%e7%95%a5%e6%a8%a1%e5%9e%8b)
  - [6. 目录结构](#6-%e7%9b%ae%e5%bd%95%e7%bb%93%e6%9e%84)
    - [6.1. include](#61-include)
        - [Core：核心数据类型](#core%e6%a0%b8%e5%bf%83%e6%95%b0%e6%8d%ae%e7%b1%bb%e5%9e%8b)
        - [IPC：进程间通讯，用于交易引擎与策略的通讯](#ipc%e8%bf%9b%e7%a8%8b%e9%97%b4%e9%80%9a%e8%ae%af%e7%94%a8%e4%ba%8e%e4%ba%a4%e6%98%93%e5%bc%95%e6%93%8e%e4%b8%8e%e7%ad%96%e7%95%a5%e7%9a%84%e9%80%9a%e8%ae%af)
        - [Utils：一些通用的功能](#utils%e4%b8%80%e4%ba%9b%e9%80%9a%e7%94%a8%e7%9a%84%e5%8a%9f%e8%83%bd)
    - [6.2. src](#62-src)
        - [Gateway](#gateway)
        - [TradingSystem](#tradingsystem)
        - [RiskManagement](#riskmanagement)
        - [Strategy](#strategy)
        - [Tools](#tools)
        - [Test](#test)
  - [7. 使用方式](#7-%e4%bd%bf%e7%94%a8%e6%96%b9%e5%bc%8f)
    - [7.1. 编译](#71-%e7%bc%96%e8%af%91)
    - [7.2. 配置登录信息](#72-%e9%85%8d%e7%bd%ae%e7%99%bb%e5%bd%95%e4%bf%a1%e6%81%af)
    - [7.3. 让示例跑起来](#73-%e8%ae%a9%e7%a4%ba%e4%be%8b%e8%b7%91%e8%b5%b7%e6%9d%a5)
  - [8. 开发你的第一个策略](#8-%e5%bc%80%e5%8f%91%e4%bd%a0%e7%9a%84%e7%ac%ac%e4%b8%80%e4%b8%aa%e7%ad%96%e7%95%a5)
  - [9. 依赖](#9-%e4%be%9d%e8%b5%96)

## 1. 更新说明
| 更新时间  | 更新内容                                                                                                      |
| --------- | ------------------------------------------------------------------------------------------------------------- |
| 2020.5.24 | redis中交易相关的键值对都带上了account id的简称，在策略加载时也需要指定account id，表示该策略挂载在哪个账户上 |

## 2. 运行示例
下面是一个简单的演示图：
![image](img/demo.jpg)
<center>图2.1 trading-engine和strategy运行示例</center>
<br>

* 左边终端运行了一个trading engine (交易引擎)。交易引擎的作用是对策略的订单转发到相应的交易接口，以及处理订后续的单回报。这期间需要经过风险管理、订单管理、仓位管理、资金管理等模块。交易引擎所管理的内容大部分都会同步到redis，策略可通过redis获取相关的信息。同时，策略也会对行情进行一个转发，通过redis发布给策略，以此驱动策略运行。左图中可以看到交易引擎刚刚启动并进行了相应的初始化，然后收到了策略发来的订单，之后收到了CTP发来的成交回报。
* 右图运行的是策略程序，策略程序和交易引擎分别属于不同的进程，一个交易引擎可同时处理多个策略的订单，通过redis与策略进行交互。策略收到交易引擎从redis推送的tick数据后，会触发on_tick回调。右图运行的是一个简单的策略，当收到一个tick数据时就buy_open一次，可以看到策略发出订单后很快就收到了来自交易引擎转发过来的订单回报。

## 3. 简介
### 3.1. FT 是什么
FT是一个基于C++的低延迟交易系统，它包含两部分，一是交易引擎，二是策略，可以看作是C/S架构的，交易引擎作为服务端处理来自策略的请求，同时为策略提供交易信息维护、数据推送等服务；而策略则是客户端，根据算法交易规则向服务端发送订单请求，或是查询交易引擎所维护的交易相关的信息（订单、仓位等）。以下是一些列举的特性：
* 采用C/S的架构，策略作为客户端不需要处理繁琐的业务逻辑，只需要专注于算法
* 一个策略对应一个进程，使得策略能高效地处理计算逻辑
* 引擎和策略端均由C++实现，掌控性能。策略也提供了python开发接口，用于快速开发部署或是策略研发
* 提供便捷的开发接口，易于对接其他交易接口
* 提供模拟的交易接口，可通过该模拟接口进行回测
* 支持多种策略模型

### 3.2. 基本架构
如图3.1所示，FT采用交易引擎与策略分离的体系，即策略引擎与交易引擎是不同的进程，通过redis进行交互。交易引擎为策略提供仓位管理、订单管理、风险管理、订单管理、资金管理、行情推送等功能，策略则可以是基于数据或是订单驱动的，也可是基本面驱动的，这个掌控权完全在策略编写端。同时对接的交易接口被抽象为Gateway，要扩展新的Gateway只需要满足Gateway的开发规范即可。
![image](img/framework.png)
<center>图3.1 整体架构图</center>

从上图可看出，对于交易引擎可分为3个主要模块
1. 交易信息管理模块：将订单转发给其他模块，同时管理相关的订单、仓位、资金等信息
2. 风险管理模块：对订单做合规管理等
3. 交易网关模块：用于对接各大交易接口

而对于策略方面，则需要关注如何通过redis与交易引擎进行交互，即协议的解析

## 4. 交易引擎模块设计
### 4.1. 模块之间的交互
订单和订单回报把各个模块关联了起来，下图描述的是从发送订单到接收到订单，各个模块之间是如何协调运行的

![image](img/Trading.png)
<center>图4.1 交易引擎模块交互</center><br>

1. 交易管理模块（TM）收到策略发来的订单后，把订单发给风险管理模块（RM），如RM模块拒绝则直接回复策略订单完结
2. 订单通过RM后，TM把订单转发给交易网关（Gateway），如果被Gateway拒绝，则立即告知RM及策略订单完结
3. 订单通过Gateway发送出去之后，TM更新仓位、资金、订单信息（下面统称交易信息），并立即通知RM
4. 如果收到交易柜台或是交易所的拒单，Gateway把拒绝的回报转发给TM，TM回滚交易信息，并立即通知RM及策略订单完结
5. 订单被交易所接纳后，Gateway把订单回执转发给TM，由TM通知RM及策略
6. 订单有成交后，Gateway收到成交回执，然后把回执转发给TM，TM更新交易信息，并通知RM及策略
7. 订单因为撤单或全成而完结后，TM更新交易信息，并通知RM及策略订单完结

### 4.2. 交易管理模块
#### 4.2.1. 接口设计
TM关心以下事件：
```cpp
class TradingEngineInterface {
 public:
  // 查询到合约时回调，这个现在不用实现
  // 因为合约都是在启动时从文件加载了，不需要启动时查询
  virtual void on_query_contract(const Contract* contract) {}

  // 查询到账户信息时回调，查询资金账户信息后回调
  // 通常在初始化时查询一次即可，后续的资金由本地计算
  // 本地计算可能存在小小的偏差，但影响不会太大
  virtual void on_query_account(const Account* account) {}

  // 查询到仓位信息时回调
  // 也只是在启动时查询一次，后续通过本地计算
  // 仓位信息应该保存在共享内存或redis等IPC中供策略查询
  virtual void on_query_position(const Position* position) {}

  // 查询到成交信息时回调
  // 也只是在启动时查询一次用于统计一些当日的交易信息
  virtual void on_query_trade(const Trade* trade) {}

  // 有新的tick数据到来时回调
  // 应该通过IPC通知策略以驱动策略运行
  // 由Gateway回调
  virtual void on_tick(const TickData* tick) {}

  // 订单被交易所接受时回调（如果只是被柜台而非交易所接受则不回调）
  // 由Gateway回调
  virtual void on_order_accepted(uint64_t order_id) {}

  // 订单被拒时回调
  // 由Gateway回调
  virtual void on_order_rejected(uint64_t order_id) {}

  // 订单成交时回调
  // 由Gateway回调
  virtual void on_order_traded(uint64_t order_id, int this_traded, double traded_price) {}

  // 撤单成功时回调
  // 由Gateway回调
  virtual void on_order_canceled(uint64_t order_id, int canceled_volume) {}

  // 撤单被拒时回调
  // 由Gateway回调
  virtual void on_order_cancel_rejected(uint64_t order_id) {}
};
```

#### 4.2.2. 订单转发
实时地监听策略通过IPC发来的下单或撤单信息，并交由RM及Gateway处理。伪代码如下：
```python
def listen_and_trasmit_order():
  while True:
    order_req = wait_order_req_from_strategy()
    if RM.check_order(order_req):
      continue
    if not Gateway.send(order_req):
      notify_RM()
      continue
    update_position()
    update_orders()
```

#### 4.2.3. 仓位管理
仓位分为多头仓位和空头仓位，目前多空的仓位信息主要有以下几个字段：
1. 当前持仓量
2. 当前待平仓量（订单已发出而未收到成交回报）
3. 当前待开仓量（订单已发出而未收到成交回报）
4. 持仓成本
5. 浮动盈亏

以下情形下需要对仓位信息进行更新：
1. 交易引擎初始化时，查询到仓位后，需要对仓位信息进行初始化设置
2. 订单通过Gateway发送成功后，需要对待开平量进行更新
3. 收到成交回报后，需要对待开平量、持仓量以及持仓成本进行更新
4. 收到撤单回报后，需要对待开平量进行更新
5. 收到tick数据后，需要对浮动盈亏进行更新。这个可选，因为tick数据变动频繁，频繁地对仓位进行更新可能会影响性能，目前没有对浮动盈亏进行更新，策略需要的话可自行计算

#### 4.2.4. 订单管理
TM需要保存未完成的订单信息，以供外界进行查询以及撤单等操作。目前订单信息保存在map中，以Gateway返回的order_id为key，订单的详细状态结构体为value。以下情形需要更新订单的map：
1. 订单通过Gateway发送成功后，将订单信息添加到map中
2. 收到订单成交回执后，如发现订单已完成，将订单信息从map中移除
3. 收到订单的撤单回执后，如发现订单已完成，将订单信息从map中移除

#### 4.2.5. 资金管理
这部分还没有设计及实现

### 4.3. 风险管理模块
TM模块通过engine_order_id告知RM是哪个订单。RM关心以下4个事件：
1. 订单发送前的检查。订单发送前RM需要对订单进行一些检查，比如如节流率检查、自成交合规检查等等，当不符合发送条件时，应该返回false将订单拦截。如果订单通过，RM可以保存或统计该订单信息，用于后续订单的风险检查
2. 订单发送成功。订单通过Gateway发送成功后，RM需要知道订单已经发送成功了，以更新RM所管理的一些订单或统计信息状态
3. 订单成交。订单成交应告知RM，用以更新RM的上下文
4. 订单完结。订单发送失败或是被拒单都应该告知RM，以更新RM的上下文

风控管理的接口如下所示：
```cpp
class RiskManagementInterface {
 public:
  // 订单发送前检查。返回false拦截订单，返回true表示该订单通过检查
  // req里面带有engine_order_id
  virtual bool check_order_req(const OrderReq* req);

  // 订单发送成功后回调
  virtual void on_order_sent(uint64_t engine_order_id);

  // 订单成交后回调
  virtual void on_order_traded(uint64_t engine_order_id, int this_traded, double traded_price);

  // 订单完结后回调
  virtual void on_order_completed(uint64_t engine_order_id);
};
```
### 4.4. 交易网关模块
交易网关作为介于TM和柜台之间的一个模块，起到承上启下的作用。Gateway的接口中，**下单和撤单需要保证线程安全**，其他接口均只在初始化时调用一次故不用保证线程安全。**除了下单和撤单操作，其他操作需要保证是同步的**，即等所有查询回执返回并处理完后接口函数才能返回。Gateway的接口说明如下所示：
```cpp
class Gateway {
 public:
  // 根据配置登录到交易柜台或行情服务器或二者都登录
  // Gateway只登录一次，可以不用做安全性保证
  virtual bool login(const Config& config) { return false; }

  // 登出
  virtual void logout() {}

  // 发单成功返回大于0的订单号，这个订单号可传回给gateway用于撤单。发单失败则返回0
  virtual uint64_t send_order(const OrderReq* order) { return 0; }

  // 取消订单，传入的订单号是send_order所返回的，只能撤销被市场接受的订单
  virtual bool cancel_order(uint64_t order_id) { return false; }

  // 查询合约信息，对于查询到的结果应回调TradingEngineInterface::on_query_contract
  virtual bool query_contracts() { return false; }

  // 查询仓位信息，对于查询到的结果应回调TradingEngineInterface::on_query_position
  virtual bool query_positions() { return false; }

  // 查询资金账户，对于查询到的结果应回调TradingEngineInterface::on_query_account
  virtual bool query_account() { return false; }

  // 查询当日历史成交信息，对于查询到的结果应回调TradingEngineInterface::on_query_trade
  virtual bool query_trades() { return false; }
};
```
与此同时，Gateway在收到订单回报时，也应该回调TradingEngineInterface中相应的函数，具体说明在**4.2.1**章节

### 4.5. 交易引擎初始化
<br>

![image](img/登录流程.png)

## 5. 策略模型
数据驱动

![image](img/MarketDataFlow.png)

## 6. 目录结构
### 6.1. include
##### Core：核心数据类型
* Account.h  Config.h  Constants.h  Contract.h  Position.h  TickData.h  Trade.h 都是交易相关的基本的通用的数据结构
* ContractTable.h 里面是ContractTable的实现，ContractTable是交易系统的重要组成部分，为了能快速获取到合约信息，以及减少map的使用，交易系统内部都使用数值索引进行合约的查找，这就要利用到ContractTable了。ContractTable的使用需要用到合约信息文件，这个文件可以通过Tools目录下的小工具contract-collector获取到
* Protocol.h 中定义了公共协议，用于TM与Gateway的交互以及策略与交易引擎的交互
* TradingEngineInterface.h 中定义了交易引擎的接口
* Gateway.h 中定义了交易网关的接口
* RiskManagementInterface.h 中定义了风险管理模块的接口
##### IPC：进程间通讯，用于交易引擎与策略的通讯
* redis.h 中封装了hiredis同步接口中的基本功能
##### Utils：一些通用的功能
* Misc.h 一些宏定义
* StringUtils.h 字符串处理函数

### 6.2. src
##### Gateway
Gateway里是各个经纪商的交易网关的具体实现，可参考CTP Gateway的实现来对接自己所需要的交易接口。目前支持CTP、XTP、以及模拟的交易网关VirtualGateway
* Ctp：上期CTP
* Xtp：中泰XTP
* Virtual：模拟交易或回测用
##### TradingSystem
TradingSystem是本人实现的一个交易引擎，向上通过redis和策略进行交互，向下通过Gateway和交易所进行交互
##### RiskManagement
* RiskManager.h/cpp 风险管理的总入口
* RiskRuleInterface.h 风险管理规则接口，需要注册到RiskManager中
* NoSelfTrade.h/cpp 禁止自成交规则
* ThrottleRateLimit.h/cpp 节流率控制
##### Strategy
* Strategy.h 一个数据驱动的策略基类
* StrategyLoader.cpp 策略加载器
* OrderSender.h 对协议进行了封装，可以向交易引擎发送订单指令
##### Tools
一些小工具，但是很必要。主要是contract-collector，用于查询所有的合约信息并保存到本地，供ContractTable使用。要注意的是，使用contract-collector时务必只配置相关的登录信息
##### Test
一些测试用例及简单的策略实现

## 7. 使用方式
### 7.1. 编译
使用cmake进行编译整个项目
```bash
mkdir build && cd build
cmake .. && make -j8
```
编译完成后会有如下几个可执行文件或动态库：
* trading-engine：交易引擎可执行文件
* strategy-loader：策略加载器可执行文件，可通过该加载器动态加载策略程序
* contract-collector：合约文件收集器可执行文件，从柜台拉取所有合约信息到本地供其他可执行文件使用
* <text>libgrid-strategy.so</text>：示例策略的动态库，可以使用strategy-loader进行加载

### 7.2. 配置登录信息
```yml
# 可以参考config/config_template.yml
api: ctp  # api name.
front_addr: tcp://180.168.146.187:10130
md_server_addr: tcp://180.168.146.187:10131
broker_id: 9999
investor_id: 123456
passwd: 12345678
auth_code: 0000000000000000
app_id: simnow_client_test
subscription_list: [rb2009, rb2007]  # 要订阅哪些合约的市场数据，是yaml数组格式的

# 是否在启动时撤销所有未完成订单，默认为true
cancel_outstanding_orders_on_startup: true
```

### 7.3. 让示例跑起来
这里提供了一个网格策略的demo
```bash
# 在terminal 0 启动redis
redis-server  # 启动redis，必须在启动策略引擎前启动redis
```
```bash
# 在terminal 1 启动交易引擎
./trading-engine --loglevel=debug \
  --config=../config/ctp_config.yml \
  --contracts=../config/contracts.csv
```
```bash
# 在terminal 2 启动策略
./strategy-loader  --loglevel=debug \
  --contracts=../config/contracts.csv \
  --account=123456 \
  --id=grid001 \
  --strategy=libgrid-strategy.so
```
配置好并运行之后就会看到如图2.1所示的结果

## 8. 开发你的第一个策略
```c++
// MyStrategy.cpp

#include <Strategy/Strategy.h>

class MyStrategy : public ft::Strategy {
 public:
  // 策略加载完后回调
  bool on_init() override {
     // 订阅感兴趣的数据
     // 订阅之后才会在有新的行情数据后收到对应的on_tick回调
     subscribe({"rb2009"});  // 可以同时订阅多个合约
  }

  // tick数据到来时回调
  void on_tick(const ft::TickData* tick) override {
    buy_open("rb2009", 1, tick->ask[0]);
  }

  // 收到本策略发出的订单的订单回报
  void on_order_rsp(const ft::OrderResponse* order) override {
    // do sth.
  }

  void on_exit() override {
    // 暂时还没用到
  }
};

EXPORT_STRATEGY(MyStrategy);  // 导出你的策略
```
把上面的代码像网格策略demo一样编译即可通过strategy-loader进行加载了
```cmake
add_library(my-strategy SHARED MyStrategy.cpp)
target_link_libraries(my-strategy strategy ${COMMON_LIB})
```
```bash
./strategy-loader  --loglevel=debug \
  --contracts=../config/contracts.csv \
  --account=123456 \
  --id=grid001 \
  --strategy=libmy-strategy.so
```

## 9. 依赖
本代码库中用到了以下依赖，感谢这些库的开发者们：
* fmt
* getopt
* hiredis
* spdlog
* yaml-cpp
