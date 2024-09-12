原ft项目做得太垃圾，已废弃。新框架xyts功能完善、稳定、延迟低，已经实盘验证，稳定运行多年，合作可直接联系作者本人。

xyts sdk已经发布在https://github.com/xywanz/xyts-strategy-sdk

一同发布的还有一个可以跑起来的策略demo，里面放了一天的行情数据，可以回测，https://github.com/xywanz/strategy


# xyts

本篇主要介绍如何在xyts上进行策略开发，并附带介绍一些xyts的可扩展模块

作者联系方式: kevin.lau.gd@gmail.com

## 开发环境

```sh
$ lsb_release -a
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 22.04.3 LTS
Release:	22.04
Codename:	jammy

g++ --version
g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
```

说明：

- 支持g++, clang++, Apple clang++，推荐版本
  - g++ >= 11.3.0
  - clang++ >= 18
- 提供数据推送的接口，正在开发监控界面，有需要也可自己对接
- 有丰富的单元测试，确保系统正确无误地运行
- 暂时只支持使用C++编写策略

## 简介

策略的开发主要涉及到三个核心的类型
- `xyts::strategy::Strategy`
- `xyts::strategy::StrategyContext`
- `xyts::strategy::StrategyParamManager`

Strategy包含了一系列的虚回调函数，需要用户去继承并将策略逻辑实现在相应的回调函数里

StrategyContext是策略运行的上下文环境，用于策略跟实盘交易/回测环境的交互，提供了下单、撤单、订阅行情、查询成交、查询持仓、添加定时器等策略需要用到的功能

StrategyParamManager用于策略读写参数，并支持运行时从外部对策略参数进行修改、实时落地同步到json文件。用户需要按照指定格式为每个策略编写一个json文件，该文件用于定义策略参数，然后用脚本strategy_param_mgr_generator.py从该配置生成.h头文件，该头文件提供了策略访问策略参数的接口

一些特性：

- 回测和实盘使用同一套策略代码，一次编译生成的策略动态库既可用于回测，也可用于实盘
- 实盘采用多进程架构，通过共享内存传递消息，各个策略可随时启停。策略之间互不影响，但可通过消息队列来订阅发布消息

一些缺陷：

- 为了不增加额外的负担，StrategyContext提供的函数都不是线程安全的，只能够用在策略构造函数、析构函数以及各类回调（包括通过ctx添加的定时器）中。如果需要使用多线程，建议通过线程安全的消息队列传递消息，策略通过定时任务定期处理消息
- 回测程序并不能很好地支持用户自己创建的多线程，需要用户自己来保证其余线程时间上的同步


## 典型策略程序示例

以一个常见的跨期套利策略为例，来说明策略的整体开发流程。这个策略并不包含错误处理，我们假设订单最后都能成功报出去。假设我们的策略名字叫做strategy_spread_arb

首先创建策略的配置文件strategy_spread_arb.json，我们在配置文件中定义了四个策略参数，分别是套利两条腿的合约名、套利上下轨的位置
```json
{
  "leg1_instr": {
    "type": "string",
    "value": "FUT_SHFE_rb-202405"
  },
  "leg2_instr": {
    "type": "string",
    "value": "FUT_SHFE_rb-202410"
  },
  "upper_line": {
    "type": "double",
    "value": 30
  },
  "lower_line": {
    "type": "double",
    "value": 20
  }
}
```

接着用strategy_param_mgr_generator.py来生成C++的策略参数访问接口

```sh
python3 strategy_param_mgr_generator.py strategy_spread_arb.json
```

运行后会在当前目录生成strategy_spread_arb.h，该文件内容如下

```cpp
// This file is generated from strategy_spread_arb.json. DO NOT EDIT!
#pragma once

#include <string>
#include <vector>

#include "xyts/strategy/strategy_param_manager.h"

class StrategySpreadArbParamManager final : public ::xyts::strategy::StrategyParamManager {
 public:
  explicit StrategySpreadArbParamManager(const std::filesystem::path& param_path): StrategyParamManager(param_path) {
    leg1_instr = CheckAndGetParamValue<std::string>("leg1_instr");
    leg2_instr = CheckAndGetParamValue<std::string>("leg2_instr");
    upper_line = CheckAndGetParamValue<double>("upper_line");
    lower_line = CheckAndGetParamValue<double>("lower_line");
  }

  const char* nameof_leg1_instr() const { return "leg1_instr"; }
  const std::string& get_leg1_instr() const { return leg1_instr; }
  void set_leg1_instr(const std::string& _leg1_instr) {
    leg1_instr = _leg1_instr;
    json_params_["leg1_instr"]["value"] = _leg1_instr;
  }

  const char* nameof_leg2_instr() const { return "leg2_instr"; }
  const std::string& get_leg2_instr() const { return leg2_instr; }
  void set_leg2_instr(const std::string& _leg2_instr) {
    leg2_instr = _leg2_instr;
    json_params_["leg2_instr"]["value"] = _leg2_instr;
  }

  const char* nameof_upper_line() const { return "upper_line"; }
  const double get_upper_line() const { return upper_line; }
  void set_upper_line(const double _upper_line) {
    upper_line = _upper_line;
    json_params_["upper_line"]["value"] = _upper_line;
  }

  const char* nameof_lower_line() const { return "lower_line"; }
  const double get_lower_line() const { return lower_line; }
  void set_lower_line(const double _lower_line) {
    lower_line = _lower_line;
    json_params_["lower_line"]["value"] = _lower_line;
  }

  void Update(const nlohmann::json& update_json) final {
    if (ContainsParam(update_json, "leg1_instr")) {
      auto value = GetParamValue<std::string>(update_json, "leg1_instr");
      set_leg1_instr(value);
    }
    if (ContainsParam(update_json, "leg2_instr")) {
      auto value = GetParamValue<std::string>(update_json, "leg2_instr");
      set_leg2_instr(value);
    }
    if (ContainsParam(update_json, "upper_line")) {
      auto value = GetParamValue<double>(update_json, "upper_line");
      set_upper_line(value);
    }
    if (ContainsParam(update_json, "lower_line")) {
      auto value = GetParamValue<double>(update_json, "lower_line");
      set_lower_line(value);
    }
  }

 private:
  template <class T>
    T CheckAndGetParamValue(const std::string& param_name) {
    if (!ContainsParam(json_params_, param_name)) {
      throw std::runtime_error("Parameter '" + param_name + "' does not exist");
    }
    return GetParamValue<T>(json_params_, param_name);
  }

  static bool ContainsParam(const nlohmann::json& json, const std::string& param_name) {
    return json.contains(param_name) && json[param_name].contains("value");
  }

  template <class T>
  static T GetParamValue(const nlohmann::json& json, const std::string& param_name) {
    return json[param_name]["value"].get<T>();
  }

  std::string leg1_instr;
  std::string leg2_instr;
  double upper_line = 0;
  double lower_line = 0;
};
```

我们在策略代码中可以通过StrategySpreadArbParamManager这个类来访问刚刚我们在json文件中定义的4个参数。这个类的实例是如何构造出来的我们不需要管，因为StrategyContext会自动帮我们构造好，我们只需要知道如何使用即可，这个会在策略实现部分继续说明。

接下来我们看看策略的实现部分，创建策略源文件strategy_spread_arb.cpp
```cpp
// 以下三个头文件中包含的分别是我们刚刚提到的三个核心类型
#include "xyts/strategy/strategy.h"
#include "xyts/strategy/strategy_context.h"
#include "strategy_spread_arb_param_manager.h"
// 日志库
#include "xyts/core/log.h"
// 合约表
#include "xyts/core/contract_table.h"
// 项目里还有很多实用的库
// #include "xyu/datetime.h"  Python-like datetime库
// ...

using namespace xyts;
using namespace xyts::strategy;

class StrategySpreadArb final : public Strategy {
 public:
  // 构造函数必须如下所示，只传入一个StrategyContext指针。
  // 我们需要将ctx保存下来，用于接下来与交易环境进行交互。
  // 构造函数就相当于策略的初始化，构造策略时策略上下文已经完成了初始化。
  explicit StrategySpreadArb(StrategyContext* ctx);

  ~StrategySpreadArb();

  void OnDepth(const DepthData& depth) final;

  // 订单状态有更新
  void OnOrder(const OrderResponse& order) final;

 private:
  StrategyContext* ctx_;                  // 用于保存StrategyContext
  StrategySpreadArbParamManager* param_;  // 策略参数

  ContractPtr leg1_contract_;
  ContractPtr leg2_contract_;

  double leg1_mid_price_ = std::numeric_limits<double>::quiet_NaN();
  double leg2_mid_price_ = std::numeric_limits<double>::quiet_NaN();
};

StrategySpreadArb::StrategySpreadArb(StrategyContext* ctx)
    : ctx_(ctx),
      param_(ctx->GetDerivedParamManager<StrategySpreadArbParamManager>()),
      leg1_contract_(ContractTable::GetByInstrument(param_->get_leg1_instr())),
      leg2_contract_(ContractTable::GetByInstrument(param_->get_leg2_instr())) {
  if (!leg1_contract_) {
    throw std::runtime_error("Unknown leg1 " + param_->get_leg1_instr());
  }
  if (!leg2_contract_) {
    throw std::runtime_error("Unknown leg2 " + param_->get_leg2_instr());
  }

  // 订阅leg1和leg2的行情和持仓信息
  ctx_->SubscribeMarketData({leg1_contract_->instr, leg2_contract_->instr});
}

StrategySpreadArb::~StrategySpreadArb() {
  LOG_INFO("Strategy {} stopped", ctx_->GetStrategyName());
  // 可以在析构函数中保存策略的一些状态到文件中，用于下次启动时加载
  // do sth.
}

void StrategySpreadArb::OnDepth(const DepthData& depth) {
  // 竞价阶段bid[0]==ask[0]，我们不在该阶段交易
  if (std::abs(depth.bid[0] - depth.ask[0]) < 1e-6) {
    return;
  }

  // 这里没有考虑bid或ask没有挂单的情况，假设bid和ask价格都存在
  if (depth.contract_id == leg1_contract_->contract_id) {
    leg1_mid_price_ = (depth.bid[0] + depth.ask[0]) / 2;
  } else {
    // 为了简单，只在收到leg2行情的时候计算spread
    leg2_mid_price_ = (depth.bid[0] + depth.ask[0]) / 2;
    // leg1还没收到行情，无法计算spread
    if (std::isnan(leg1_mid_price_)) {
      return;
    }
    double spread = leg1_mid_price_ - leg2_mid_price_;
    auto pos = ctx_->GetLogicalPosition(leg1_contract_->contract_id);
    if (spread >= param_->get_upper_line()) {
      // 超过上轨，如果仓位还没满做空spread
      if (pos.volume > -1) {
        ctx_->Buy(leg2_contract_->contract_id, 1, OrderType::kMarket, 0);
        ctx_->Sell(leg1_contract_->contract_id, 1, OrderType::kMarket, 0);
      }
    } else if (spread <= param_->get_lower_line()) {
      // 跌破下轨，如果仓位还没满则做多spread
      if (pos.volume < 1) {
        ctx_->Sell(leg2_contract_->contract_id, 1, OrderType::kMarket, 0);
        ctx_->Buy(leg1_contract_->contract_id, 1, OrderType::kMarket, 0);
      }
    }
  }
}

void StrategySpreadArb::OnOrder(const OrderResponse& order) {
  const auto* contract =
      order.contract_id == leg1_contract_->contract_id ? leg1_contract_ : leg2_contract_;
  LOG_INFO("{} {}{} {} {} px:{:.2f} fill/total:{}/{} order_id:{}", contract->instr, order.direction,
           order.position_effect, order.status, order.error_code, order.price,
           order.accum_trade_volume, order.original_volume, order.order_id);

  if (order.current_trade_volume > 0) {
    LOG_INFO("{} {}{} {:.2f}@{}", contract->instr, order.direction, order.position_effect,
             order.current_trade_price, order.current_trade_volume);
  }
}

// 导出策略符号，使得能通过动态库的形式加载
EXPORT_STRATEGY(StrategySpreadArb);
```

编写完后将策略编译成动态库即可被实盘或回测环境加载

## 策略程序编译

## 策略程序回测

我们使用backtester程序对策略进行回测

我们需要准备以下数据或程序用于回测，目录结构如下

```sh
├── bin
│   └── backtester
├── conf
│   └── backtester.yaml
├── data
├── lib
│   ├── libstrategy_spread_arb.so
│   └── libxyts.so
└── log
```

我们在conf下创建backtester.yaml，并填入如下配置

```yaml
strategies:
  - strategy_name: strategy_spread_arb
    strategy_so: ../lib/libstrategy_spread_arb.so  # 编译好的策略动态库
    strategy_param_file: ../conf/strategy_spread_arb.json

# 回测范围为[begin_date, end_date]之间的所有交易日
backtest_period:
  begin_date: 2024-01-01
  end_date: 2024-01-15

strategy_start_time:
  night: '20:55'  # 夜盘策略启动时间，默认值20:50
  day: '08:55'  # 日盘策略启动时间，默认值08:50

sessions:
# 夜盘交易时段
  night:
    - begin_time: '20:59'
      end_time: '23:01'
  # 日盘交易时段
  day:
    - begin_time: '09:00'
      end_time: '10:15'
    - begin_time: '10:30'
      end_time: '11:30'
    - begin_time: '13:30'
      end_time: '15:01'

initial_capital: 100000000  # 起始资金

# 合约表路径
contract_files:
  - ../data/contracts_%Y%m%d.csv

holiday_directory: ../data/holiday  # xydata交易日历路径

# 行情数据配置
data_feed:
  type: csv_data_feed
  data_directory: ../data/depth  # xydata csv格式的快照数据目录

matching_engine:
  type: emu_matching_engine  # emu为排队撮合

# 手续费配置
fee:
  - instr: 'FUT_SHFE_rb.+?'
    type: percentage
    rate: 0.0001

# 滑点
slippage: 0
```

准备回测需要用到的交易日历、快照以及合约表数据，放在data目录中。数据如何组织以及数据的格式请参考`xydata`

准备好数据后，进入到bin目录中，执行命令启动回测

```sh
nohup ./backtester ../conf/backtester.yaml ../log/backtester.log 2>&1 &
```

日志会输出到log目录下，回测完成后会将回测结果输出到backtest_stats.json

## 策略实盘上线

实盘用strategy_loader来加载我们编译好的策略动态库。

在启动策略前需要启动以下进程：

- trader
- market_center
- 各个data_feed程序用于收行情（只需要在market_center之后启动，与策略之间的启动顺序没有要求，而且可以随时启停），如果有多个源的话可以启动多个以提高可靠性

配置好交易系统及策略后，可以在bin目录下执行以下命令启动交易系统
```sh
# 生成合约表
./query_contracts ../log/trader.log ../conf
# 一键启动trader、market_center以及各个行情程序
./restart.sh
# 启动策略
python3 manager.py start_strategy strategy_spread_arb_01
```

## Strategy基类

策略是由事件驱动，策略目前支持以下几种事件

- Depth
- Order
- Message
- ParamUpdate
- Command

### OnDepth

如果策略在初始化时订阅了一些合约，那么这些合约的行情更新时会通过OnDepth通知策略，策略可在OnDepth中实现策略的主要逻辑

### OnOrder

策略订单的状态发生任何变动时，都会通过OnOrder通知策略，具体的变动包括

- 订单被XYTS风控拒绝
- 订单被XYTS接收
- 订单被柜台/交易所拒绝
- 订单被交易所接收
- 订单被撤销
- 订单发生了成交（每有一笔成交都会通知一次）

### OnParamUpdate

策略参数由外部进行修改时，会由该回调通知，消息中只包含了那些被修改的参数。假如策略的upper_line和lower_line两个参数被外部修改了，消息格式如下

```json
{
  "upper_line": {
    "value": 32
  },
  "lower_line": {
    "value": 22
  }
}
```

### OnMessage

如果策略在初始化时调用了SubscribeTopics订阅了一些topics的话，如果该topics中有策略感兴趣数据进来，会通过OnMessage通知策略，策略需要自行对数据进行解包

也可以通过OnMessage接收来自data_feed_api的自定义行情

### OnCommand

策略收到自定义控制命令

## StrategyContext用法说明

### AddTimeout & AddPeriodicCallback

XYTS内置高精度定时器功能，可以在策略构造时或任意回调里使用，并支持取消。该功能完美支持回测。

单次的超时回调

```cpp
// 500微秒后执行1次
auto timer_id = ctx->AddTimeout(std::chrono::microseconds{500}, [this](auto) {
  // do sth.
});
// 如果不想在500微秒后执行了，可以移除定时器
ctx->RemoveTimer(timer_id);
// 可以支持多种时间类型
ctx->AddTimeout(std::chrono::milliseconds{1}, [this](auto) {});
ctx->AddTimeout(std::chrono::seconds{1}, [this](auto) {});
```

定期执行的回调

```cpp
// 每500微秒执行1次
auto timer_id = ctx->AddPeriodicCallback(std::chrono::microseconds{500}, [this](auto id) {
  // do sth.
  // 不需要了的话既可以在外面取消，也可以在内部取消
  // ctx->RemoveTimer(id);
});
// 如果不需要了，可以移除定时器
ctx->RemoveTimer(timer_id);
// 可以支持多种时间类型
ctx->AddPeriodicCallback(std::chrono::milliseconds{1}, [this](auto) {});
ctx->AddPeriodicCallback(std::chrono::seconds{1}, [this](auto) {});
```

### SubscribeMarketData

SubscribeMarketData用于订阅行情，一般在策略构造函数中调用，策略初始化时就应该知道该订阅哪些合约，不建议在策略运行过程中调用，否则会导致策略回测时无法加载行情数据

```cpp
MyStrategy::MyStrategy(StrategyContext* ctx) {
  ctx->SubscribeMarketData({"FUT_SHFE_rb-202405", "FUT_SHFE_rb-202410"});  // 支持指定具体合约
  ctx->SubscribeMarketData({"FUT_SHFE_rb.+?"});  // 支持使用正则表达式，匹配规则与std::regex相同
  ctx->SubscribeMarketData({"FUT_SHFE_rb.+?", "FUT_SHFE_hc-202405"});  // 可以混合使用
}
```

### UnsubscribeMarketData

取消订阅行情

```cpp
MyStrategy::MyStrategy(StrategyContext* ctx) {
  ctx->UnsubscribeMarketData({"FUT_SHFE_rb-202405", "FUT_SHFE_rb-202410"});  // 支持指定具体合约
  ctx->UnsubscribeMarketData({"FUT_SHFE_rb.+?"});  // 支持使用正则表达式，匹配规则与std::regex相同
  ctx->UnsubscribeMarketData({"FUT_SHFE_rb.+?", "FUT_SHFE_hc-202405"});  // 可以混合使用
}
```

### SubscribePosition

订阅物理持仓，只有订阅了物理持仓信息才能通过GetPosition读取到正确的物理持仓。逻辑持仓不受此影响，不管订阅与否策略都能读取到正确的逻辑持仓

```cpp
ctx->SubscribePosition({"FUT_SHFE_rb-202405"});
```

### UnsubscribePosition

策略可以取消订阅物理持仓

```cpp
ctx->UnsubscribePosition({"FUT_SHFE_rb-202405"});
```

### SubscribeTopics

XYTS提供了基于共享内存的低延迟消息队列，策略可以订阅感兴趣的topic，之后如有新消息会通过OnMessage进行推送

```cpp
ctx->SubscribeTopics({1, 3, 5});
```

### UnsubscribeTopics

策略可以取消订阅topic

```cpp
ctx->UnsubscribeTopics({1, 3, 5});
```

### GetWallTime

获取当前微秒精度的时间戳，实盘时返回的是当前时间，回测返回的回测时模拟的时间。尽可能使用该接口来获取时间，以确保实盘和回测行为能保持一致。另外，GetWallTime可配合xyu/datetime.h使用以获取更多功能

```cpp
#include "xyu/datetime.h"

namespace dt = xyu::datetime;

// ...

auto ts = ctx->GetWallTime();
auto now = dt::datetime::fromtimestamp(ts);
LOG_INFO("{}", now.strftime("%Y-%m-%d %H:%M:%S.%f"));
```

### SendAlarm

向外推送告警信息，可支持推送至企业微信

```cpp
ctx->SendAlarm("Hello");
```

### Stop

停止策略并退出程序，用于遇到BUG或是其他紧急情况下策略内部主动退出

```cpp
ctx->Stop();
```

### SendOrder

支持订单超时参数，超时会自动撤销订单

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
  // 设置了100ms的超时
  // 返回的client_order_id可对订单进行操作，如撤单、修改订单超时等，也可以用于在回报中识别订单
auto cli_order_id =
    ctx->SendOrder(contract->contract_id, 1, Direction::kBuy, PositionEffect::kAuto,
                   OrderType::kLimit, 3900, std::chrono::milliseconds{100});
```

### Buy

对SendOrder的简单封装，PositionEffect为PositionEffect::kAuto

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
ctx->Buy(contract->contract_id, 1, OrderType::kLimit, 3900);
```

### Sell

对SendOrder的简单封装，PositionEffect为PositionEffect::kAuto

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
ctx->Sell(contract->contract_id, 1, OrderType::kLimit, 3900);
```

### CancelOrder

撤单，调用一次即可保证订单立刻结束

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
auto cli_order_id =
    ctx->SendOrder(contract->contract_id, 1, Direction::kBuy, PositionEffect::kAuto,
                   OrderType::kLimit, 3900, std::chrono::milliseconds{100});
ctx->CancelOrder(cli_order_id);
```

### ResetOrderTimeout

重置订单超时时间

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
auto cli_order_id =
    ctx->SendOrder(contract->contract_id, 1, Direction::kBuy, PositionEffect::kAuto,
                   OrderType::kLimit, 3900, std::chrono::milliseconds{100});
// 重新设置成1s，超时是从设置的那一刻开始计算
ctx->ResetOrderTimeout(cli_order_id, std::chrono::seconds{1});
```

### GetPosition

获取单个合约的实时物理持仓，效率很高

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
auto pos = ctx->GetPosition(contract->contract_id);
```

### GetPositions

查询所有物理持仓，因为涉及到拷贝，如果合约量较大性能会稍差

```cpp
auto positions = ctx->GetPositions();
for (const auto& position : positions) {
  // ...
}
```

### GetLogicalPosition

获取单个合约的实时逻辑持仓，效率很高

```cpp
auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
auto pos = ctx->GetLogicalPosition(contract->contract_id);
```

### GetLogicalPositions

查询所有逻辑持仓，因为涉及到拷贝，如果合约量较大性能会稍差

```cpp
auto positions = ctx->GetLogicalPositions();
for (const auto& position : positions) {
  // ...
}
```

### GetTrades

从数据库读取策略当日成交，性能较差，只适合在策略初始化时查询。如果要维护实时的成交列表，需要配合OnOrder回调使用

```cpp
auto trades = ctx->GetTrades();  // 获取策略所有成交

auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
auto rb2405_trades = ctx->GetTrades(contract->contract_id);  // 获取策略在rb2405上的成交
```

### GetOrders

从数据库读取策略当日订单信息，性能较差，只适合在策略初始化时查询。如果要维护实时的订单列表，需要配合OnOrder回调使用

```cpp
auto orders = ctx->GetOrders();  // 获取策略所有成交

auto contract = ContractTable::GetByInstrument("FUT_SHFE_rb-202405");
auto rb2405_orders = ctx->GetOrders(contract->contract_id);  // 获取策略在rb2405上的委托
```

### GetAccount

查询帐户资金信息，这个一般用得比较少，需要填入配置的帐户名称

```cpp
auto account = GetAccount("my_ctp_account");
```

### GetPnl

查询策略从上线到目前为止的最新的累积pnl，如果有持仓需要订阅对应合约的行情才能获取到正确的值，如果GetPnl返回nan，说明还没有足够的行情用于计算pnl，需要继续收取行情

```cpp
double pnl = GetPnl();
```

### PublishMessage

策略可以通过PublishMessage往消息队列推送消息

data的最大长度有限制，定义在xyts/core/market_data.h的kMaxTopicMessageLen，超出的话接口会抛异常

```cpp
Topic topic = 1;
const char* data = "hello, world";
ctx->PublishMessage(topic, data, strlen(data) + 1);
```

### GetStrategyName

获取当前策略实例的名称

```cpp
LOG_INFO("{}", ctx->GetStrategyName());
```

### GetParamManager

获取策略参数对象，通常需要用dynamic_cast转换成相应的派生类型，参考`典型策略程序示例`这一章节的示例

### GetDerivedParamManager

用dynamic_static对GetParamManager进行的简单封装

## 高级订单管理功能

高级订单管理功能提供了如套利、追踪止损、条件单等功能

### OrderManager

OrderManager能同时在两个价位上分别挂上买单和卖单

```cpp
#include "xyts/strategy/order_manager.h"
// include other necessary header files

class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        order_manager(ctx_) {
    ctx_->SubscribeMarketData({param_->get_instr()});
  }

  void OnDepth(const DepthData& depth) final {
    // 每次行情更新都会先撤销掉原来的订单，并在买1和卖1分别挂一手订单
    order_manager_.PlaceOrder(depth.contract_id, 1, depth.bid[0], 1, depth.ask[0]);
  }

  void OnOrder(const OrderResponse& rsp) final {
    order_manager_.OnOrder(rsp);
  }

 private:
  StrategyContext ctx_;
  MyStrategyParamManager* param_;
  OrderManager order_manager_;
};
```

### ArbitrageManager

### TargetPositionManager

策略通过TargetPositionManager设置目标持仓量后，TargetPositionManager会自动将持仓调整到目标持仓量，策略无需再处理订单逻辑。

```cpp
#include "xyts/strategy/target_position_manager.h"
// include other necessary header files

class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        contract_(ContractTable::GetByInstrument(param_->get_instr())),
        target_pos_manager_(ctx_) {
    ctx_->SubscribeMarketData({param_->get_instr()});
  }

  void OnDepth(const DepthData& depth) final {
    if (depth.last_price > 3900) {
      target_pos_maager_.SetTargetPosition(depth.contract_id, -10);
    } else if (depth.last_price < 3880) {
      target_pos_maager_.SetTargetPosition(depth.contract_id, 10);
    }
    target_pos_manager_.OnDepth(depth);
  }

  void OnOrder(const OrderResponse& order) final { target_pos_manager_.OnOrder(rsp); }

 private:
  StrategyContext* ctx_;
  MyStrategyParamManager* param_;
  ContractPtr contract_;
  TargetPositionManager target_pos_manager_;
};
```

### ConditionOrderManager

ConditionOrderManager用于支持条件单，条件单会在价格突破时触发

```cpp
#include "xyts/strategy/condition_order_manager.h"
// include other necessary header files

class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        contract_(ContractTable::GetByInstrument(param_->get_instr())),
        cond_ord_manager_(ctx_) {
    ctx_->SubscribeMarketData({param_->get_instr()});

    const auto* contract = ContractTable::GetByInstrument(param_->get_instr());
    assert(contract);
    // 价格突破3950时买入5手
    ConditionOrder order{contract->contract_id, Direction::kBuy, PositionEffect::kAuto, 5, 3950};
    cond_ord_manager_.AddConditionOrder(order);
  }
  void OnDepth(const DepthData& depth) final { cond_ord_manager_.OnDepth(depth); }

  void OnOrder(const OrderResponse& order) final { cond_ord_manager_.OnOrder(order); }

 private:
  StrategyContext* ctx_;
  MyStrategyParamManager* param_;
  ContractPtr contract_;
  ConditionOrderManager cond_ord_manager_;
};
```

### TrailingStopManager

TrailingStopManager用于追踪止损

```cpp
#include "xyts/strategy/trailing_stop_manager.h"
// include other necessary header files

class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        contract_(ContractTable::GetByInstrument(param_->get_instr())),
        trailing_stop_manager_(ctx_) {
    ctx_->SubscribeMarketData({param_->get_instr()});

    const auto* contract = ContractTable::GetByInstrument(param_->get_instr());
    assert(contract);
    // 持仓方向为多头，基准价格为3900，初始的止损价格为3880，当价格进一步上涨时，止损价格会被抬高，
    // 当最新价跌破追踪止损价格时，卖出2手来止损
    trailing_stop_manager_.AddTrailingStop(contract->contract_id, Direction::kBuy, 2, 3900, 20);
  }

  void OnDepth(const DepthData& depth) final { trailing_stop_manager_.OnDepth(depth); }

  void OnOrder(const OrderResponse& order) final { trailing_stop_manager_.OnOrder(order); }

 private:
  StrategyContext* ctx_;
  MyStrategyParamManager* param_;
  ContractPtr contract_;
  TrailingStopManager trailing_stop_manager_;
};
```

### GridTradingBot

网格交易，支持固定网格大小、固定的price_tick以及固定的价格比例

```cpp
#include "xyts/strategy/grid_trading_bot.h"
// include other necessary header files

class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        contract_(ContractTable::GetByInstrument(param_->get_instr())) {
    ctx_->SubscribeMarketData({param_->get_instr()});
    if (std::filesystem::exists(grid_trading_dump_)) {
      grid_trading_bot_ = GridTradingBot::Load(ctx, grid_trading_dump_);
    } else {
      grid_trading_bot_ = std::make_unique<GridTradingBot>(
          ctx, contract, 1, GridHeight(0.01, GridHeightType::kRatioLength));
    }
  }

  ~MyStrategy() {
    grid_trading_bot_->Dump(grid_trading_dump_);
  }

  void OnDepth(const DepthData& depth) final { grid_trading_bot_->OnDepth(depth); }

  void OnOrder(const OrderResponse& order) final { grid_trading_bot_->OnOrder(order); }

 private:
  StrategyContext* ctx_;
  MyStrategyParamManager* param_;
  ContractPtr contract_;
  std::unique_ptr<GridTradingBot> grid_trading_bot_;
  std::string grid_trading_dump_ = "../data/my_grid_trading_bot.json";
};
```

### AutoSpreader

### SpreadGridTradingBot

## 算法交易

### 使用方式

```cpp
#include "xyts/strategy/algo_trading_service.h"
// #include others

class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        contract_(ContractTable::GetByInstrument(param_->get_instr())),
        algo_trading_service_(ctx_) {
    ctx_->SubscribeMarketData({param_->get_instr()});
    ctx_->SubscribePosition({param_->get_instr()});
  }

  void OnDepth(const DepthData& depth) final {
    algo_trading_service_.OnDepth(depth);
    if (satisfied condition) {
      nlohmann::json algo_params{{"algo_name", "Iceberg"},
                                 {"instr", contract_->instr},
                                 {"direction", "Buy"},
                                 {"position_effect", "Auto"},
                                 {"volume", 100},
                                 {"timeout", 10 * 1000000},
                                 {"price_depth", 0.5},
                                 {"interval", 2 * 1000000},
                                 {"start_time", 0},
                                 {"order_type", "price_preferred"},
                                 {"display_vol", 0.2},
                                 {"price_tick_added", 0},
                                 {"reject_interval", 3 * 1000000},
                                 {"imb_sum_ratio", 0.5},
                                 {"imb_level1_ratio", 0.5},
                                 {"traded_interval", 1 * 1000000},
                                 {"allow_pending_up_limit", false},
                                 {"allow_pending_down_limit", true}};
      algo_trading_service_.AddAlgoOrder(algo_params);
    }
  }

  void OnOrder(const OrderResponse& order) final { algo_trading_service_.OnOrder(order); }

 private:
  StrategyContext* ctx_;
  MyStrategyParamManager* param_;
  ContractPtr contract_;
  AlgoTradingService algo_trading_service_;
};
```
### 支持的算法类型：

所有参数中的时间单位均是微秒

每个算法都共有的参数

```json
{
  "algo_name": "TWAP/VWAP/...",
  "instr": "000001.XSHE",
  "direction": "Buy",
  "position_effect": "Auto",
  "volume": 10000,
  "timeout": "30000000"
}
```

#### TWAP

简介：根据时间加权平均市价，间隔时间发单

参数说明:

- start_time, end_time：控制TWAP开始与结束的时间（算法可能会因成交慢完成时间滞后于给定时间）
- reject_interval:订单被拒后，间隔多久重发
- duration: 每隔多少微秒发送子单
- price_tick_added: 超时后，调整子单的价格。为当前时间twap +- price_tick_added * unit_price_tick

示例

```json
{
  "algo_name": "TWAP",
  "start_time": 1705064340608000,
  "end_time": 1705064400888000,
  "duration": 30000000,
  "reject_interval": 500000,
  "price_tick_added": 1
}
```

#### VWAP

简介：根据成交量加权平均市价，间隔时间发单

具体参数解释：和TWAP参数一致，除了duration需要最小间隔分钟，如 algo_params["duration"] = 60000000

示例

```json
{
  "algo_name": "VWAP",
  "start_time": 1705064340608000,
  "end_time": 1705064400888000,
  "duration": 60000000,
  "reject_interval": 500000,
  "price_tick_added": 1
}
```

#### IS

简介：将冲击成本与时间成本考虑在内，给定风险系数下，求最优分单

具体参数解释:

- risk_aversion：风险厌恶系数(>0)，越大表示对时间冲击成本带来风险越谨慎，故会尽可能提早完成算法单(开始时，子单大小很大)
- total_time：表示执行的总时间，duration表示间隔时间，两者最小单位均需要是分钟。

示例

```json
{
  "algo_name":"IS",
  "total_time": 60000000,
  "risk_aversion": 0.5,
  "duration": 10000000,
  "reject_interval": 500000,
  "price_tick_added": 1
}
```

#### Iceberg

简介：根据显示数量参数，在某个档位或以对价发送小额单，只有上一笔子单完成，才会继续发送下笔，直到成交量满足给定额

具体参数解释：
- order_type: 发单类型，分为两种 1.时间优先"time_preferred" 2.价格优先"price_preferred"
- display_vol: 每笔子单显示数量，如果是分数(0-1)，则按当前市场五档挂单数量均值*给定分数动态计子单数量大小;如果是整数，则按给定整数定值确定每笔子单数额
- interval: 每隔多少微秒检测子单价格是否波动太大
- reject_interval: 订单被拒后,间隔多久重发
- traded_interval: 订单完全成交后,间隔多久发下笔子单
- imb_sum_ratio: 判断买卖势力加上一个比率,在买单中,如果sum_asks/sum_bids > ratio,才有可能bid1单; 如果想要更快成交(打对价)，将该ratio调为>1的值即可，反之<1
- imb_level1_ratio: 同上,但是检测的是ask1/bid1
- allow_pending_up_limit: 涨停板下，允许一直挂单(没有超时)
- allow_pending_down_limit: 跌停板下，允许一直挂单(没有超时)
- last_order_kbest: 科创板下，零股是否发市价单
- price_depth: 检查价格深度(百分比),如果子单价格超过当前价格的2 * price_depth/100，则撤销发。（其中小市值股票根据price_tick判断）
- price_tick_added: 只在价格优先下有用,会根据price_tick_added，调整价格发出

示例

```json
{
  "algo_name":"Iceberg",
  "price_depth": 0.2,
  "interval": 2000000,
  "start_time": 1705064340608000,
  "display_vol": 0.2,
  "reject_interval": 500000,
  "traded_interval": 500000,
  "imb_sum_ratio": 0,
  "imb_level1_ratio": 0,
  "price_tick_added": 0,
  "allow_pending_up_limit": true,
  "allow_pending_down_limit": true
}
```

#### Sniper

简介：市场行情达到给定条件，则立刻发对价单，数额是对价一档挂单量，直到完成给定数额

具体参数解释：

- aggressiveness: 是时间间隔，再发送上一笔子单后，超过aggressiveness微秒后，才能发送下一笔达到条件的子单

示例

```json
{
  "algo_name":"Sniper",
  "start_time": 1705064340608000,
  "aggressiveness": 100000
}
```

## BarGenerator: 实时K线合成

BarGenerator支持实时合成多种周期的K线，如3s/5s/6s/10s/15s/20s/30s/1min/3min/5min/15min等

```cpp
class MyStrategy final : public Strategy {
 public:
  explicit MyStrategy(StrategyContext* ctx)
      : ctx_(ctx),
        param_(ctx->GetDerivedParamManager<MyStrategyParamManager>()),
        bargen_(ctx) {
    // 1min
    bargen_.AddBarPeriod({"FUT_SHFE_rb-.+?"}, std::chrono::seconds{60},
                         [this](const BarData& bar) { OnBar1Min(bar); });
    // 5min
    bargen_.AddBarPeriod({"FUT_SHFE_rb-.+?"}, std::chrono::seconds{300},
                         [this](const BarData& bar) { OnBar5Min(bar); });
  }

  void OnDepth(const DepthData& depth) final { bargen_.UpdateBar(depth); }

  void OnBar1Min(const BarData& bar) {}

  void OnBar5Min(const BarData& bar) {}

 private:
  StrategyContext* ctx_;
  MyStrategyParamManager* param_;
  BarGenerator bargen_;
};
```

下面是BarGenerator的实现，供参考学习

```cpp
// bar_generator.h
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "xyts/core/contract_table.h"
#include "xyts/core/market_data.h"
#include "xyts/strategy/strategy_context.h"

namespace xyts::strategy {

using BarCallback = std::function<void(const BarData&)>;

class BarGenerator {
 public:
  explicit BarGenerator(StrategyContext* ctx);

  ~BarGenerator();

  void AddBarPeriod(const std::vector<std::string>& patterns, std::chrono::seconds period,
                    BarCallback&& cb);

  void UpdateBar(const DepthData& depth);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace xyts::strategy
```

```cpp
// bar_generator.cpp
#include "xyts/strategy/bar_generator.h"

#include <optional>
#include <string>

#include "xydata/bar.h"
#include "xyu/datetime.h"

namespace dt = xyu::datetime;

namespace xyts::strategy {

class BarGenHelper {
 public:
  BarGenHelper(StrategyContext* ctx, ContractPtr contract, std::chrono::seconds period,
               BarCallback cb);

  ~BarGenHelper();

  void UpdateBar(const DepthData& depth);

  void CheckBar(std::chrono::microseconds now_ts);

  ContractPtr contract() const { return contract_; }

  Volume last_volume() const { return last_depth_ ? last_depth_->volume : 0; }

  double last_turnover() const { return last_depth_ ? last_depth_->turnover : 0; }

  std::chrono::seconds period() const { return bar_.period; }

 private:
  void OpenBar(const DepthData& depth);

  void CloseBar(std::chrono::microseconds bar_time);

  StrategyContext* ctx_;
  ContractPtr contract_;
  BarCallback cb_;

  std::vector<std::vector<std::chrono::microseconds>> intervals_;
  std::size_t interval_idx_ = 0;

  std::optional<DepthData> last_depth_;
  BarData bar_{};
  double open_px_ = std::numeric_limits<double>::quiet_NaN();
  double high_px_ = std::numeric_limits<double>::quiet_NaN();
  double low_px_ = std::numeric_limits<double>::quiet_NaN();
};

BarGenHelper::BarGenHelper(StrategyContext* ctx, ContractPtr contract, std::chrono::seconds period,
                           BarCallback cb)
    : ctx_(ctx), contract_(contract), cb_(cb) {
  bar_.contract_id = contract_->contract_id;
  snprintf(bar_.source, sizeof(bar_.source), "bargen");
  bar_.period = period;

  auto now_ts = ctx->GetWallTime();
  auto now = dt::datetime::fromtimestamp(now_ts);
  auto today = now.date();

  const auto* all_sessions = contract_->sessions;
  if (!all_sessions) {
    throw std::runtime_error("Failed to get sessions of " + contract_->instr);
  }
  std::vector<xydata::Session> sessions;
  for (const auto& session : all_sessions->GetAllTradingSessions()) {
    if (now.hour() >= 8 && now.hour() <= 16) {
      if (session.open.hour() >= 8 && session.open.hour() <= 16) {
        sessions.emplace_back(session);
      }
    } else {
      if (session.open.hour() >= 20 || session.open.hour() <= 3) {
        sessions.emplace_back(session);
      }
    }
  }

  auto time_intervals = xydata::SplitToBarIntervals(sessions, period);
  auto combine_trading_date = [&](const auto& t) {
    if (t >= dt::time(0) && t <= dt::time(3)) {
      return dt::datetime::combine(today + dt::timedelta(1), t).timestamp();
    } else {
      return dt::datetime::combine(today, t).timestamp();
    }
  };
  for (const auto& time_itv : time_intervals) {
    std::vector<std::chrono::microseconds> interval;
    interval.emplace_back(combine_trading_date(time_itv.begin));
    interval.emplace_back(combine_trading_date(time_itv.end));
    interval.emplace_back(combine_trading_date(time_itv.bar_time));
    intervals_.emplace_back(std::move(interval));
  }
  while (interval_idx_ < intervals_.size()) {
    if (now_ts < intervals_[interval_idx_][1] + std::chrono::seconds{1}) {
      break;
    }
    interval_idx_++;
  }
}

BarGenHelper::~BarGenHelper() {}

void BarGenHelper::UpdateBar(const DepthData& depth) {
  if (interval_idx_ >= intervals_.size()) {
    return;
  }
  if (depth.volume == 0) {
    return;
  }

  const auto& interval = intervals_[interval_idx_];
  if (depth.exchange_timestamp < interval[0]) {
    return;
  }
  if (depth.exchange_timestamp > interval[1]) {
    interval_idx_++;
    if (!last_depth_) {
      OpenBar(depth);
    } else {
      CloseBar(interval[2]);
      OpenBar(depth);
    }
  } else {
    if (!last_depth_ || std::isnan(open_px_)) {
      OpenBar(depth);
    } else {
      high_px_ = std::max(high_px_, depth.last_price);
      low_px_ = std::min(low_px_, depth.last_price);
    }
  }
  last_depth_ = depth;
}

void BarGenHelper::OpenBar(const DepthData& depth) {
  open_px_ = depth.last_price;
  high_px_ = depth.last_price;
  low_px_ = depth.last_price;
}

void BarGenHelper::CloseBar(std::chrono::microseconds bar_time) {
  if (std::isnan(open_px_)) {
    OpenBar(*last_depth_);
  }
  bar_.exchange_timestamp = bar_time;
  bar_.local_timestamp = ctx_->GetWallTime();
  bar_.volume = last_depth_->volume;
  bar_.turnover = last_depth_->turnover;
  bar_.open_price = open_px_;
  bar_.high_price = high_px_;
  bar_.low_price = low_px_;
  bar_.close_price = last_depth_->last_price;
  cb_(bar_);
}

void BarGenHelper::CheckBar(std::chrono::microseconds now_ts) {
  if (interval_idx_ >= intervals_.size()) {
    return;
  }
  const auto& interval = intervals_[interval_idx_];
  if (now_ts >= interval[1] + std::chrono::seconds{1}) {
    interval_idx_++;
    if (last_depth_) {
      CloseBar(interval[2]);
      open_px_ = std::numeric_limits<double>::quiet_NaN();
      high_px_ = std::numeric_limits<double>::quiet_NaN();
      low_px_ = std::numeric_limits<double>::quiet_NaN();
    }
  }
}

class BarGenerator::Impl {
 public:
  explicit Impl(StrategyContext* ctx);

  ~Impl();

  void AddBarPeriod(const std::vector<std::string>& patterns, std::chrono::seconds period,
                    BarCallback&& cb);

  void UpdateBar(const DepthData& depth);

 private:
  void CheckBars();

  StrategyContext* ctx_;
  std::unordered_map<ContractId, std::vector<std::shared_ptr<BarGenHelper>>> contract_to_helpers_;
  std::vector<std::shared_ptr<BarGenHelper>> all_helpers_;

  EventId timer_id_ = -1;
};

BarGenerator::Impl::Impl(StrategyContext* ctx) : ctx_(ctx) {
  timer_id_ = ctx_->AddPeriodicCallback(std::chrono::seconds{1}, [this](auto) { CheckBars(); });
}

void BarGenerator::Impl::AddBarPeriod(const std::vector<std::string>& patterns,
                                      std::chrono::seconds period, BarCallback&& cb) {
  auto contracts = ContractTable::GetByPatterns(patterns);
  for (const auto* contract : contracts) {
    auto& helpers = contract_to_helpers_[contract->contract_id];
    auto it = std::find_if(helpers.begin(), helpers.end(),
                           [period](const auto& helper) { return helper->period() == period; });
    if (it != helpers.end()) {
      continue;
    }
    auto helper = std::make_shared<BarGenHelper>(ctx_, contract, period, cb);
    helpers.emplace_back(helper);
    all_helpers_.emplace_back(helper);
  }
}

BarGenerator::Impl::~Impl() { ctx_->RemoveTimer(timer_id_); }

void BarGenerator::Impl::UpdateBar(const DepthData& depth) {
  if (auto it = contract_to_helpers_.find(depth.contract_id); it != contract_to_helpers_.end()) {
    for (const auto& helper : it->second) {
      helper->UpdateBar(depth);
    }
  }
}

void BarGenerator::Impl::CheckBars() {
  auto now_ts = ctx_->GetWallTime();
  for (const auto& helper : all_helpers_) {
    helper->CheckBar(now_ts);
  }
}

BarGenerator::BarGenerator(StrategyContext* ctx) : impl_(std::make_unique<Impl>(ctx)) {}

BarGenerator::~BarGenerator() {}

void BarGenerator::AddBarPeriod(const std::vector<std::string>& patterns,
                                std::chrono::seconds period, BarCallback&& cb) {
  impl_->AddBarPeriod(patterns, period, std::move(cb));
}

void BarGenerator::UpdateBar(const DepthData& depth) { impl_->UpdateBar(depth); }

}  // namespace xyts::strategy
```

## 实盘MarketDataFilter扩展

实盘中，如果同时接入了多个相同的行情源，用户可加载自定义的行情过滤器，xyts默认提供了两个过滤器

- duplicate_filter: 行情择优去重
- timeout_filter: 如果行情的交易所时间戳比本地接收时间戳小得多，则丢弃该行情

在market_center.yaml中配置filter即可使用

```yaml
market_data_filters:
  - name: duplicate_filter
    errata_ms: 50
  - name: timeout_filter
    timeout_ms: 5000
```

MarketDataFilter支持扩展，用户可将自定义的MarketFilter编译成so放在lib下即可加载

```cpp
#pragma once

#include <chrono>

#include "xyts/market_data_filter/market_data_filter.h"

namespace xyts {

class MyFilter final : public MarketDataFilter {
 public:
  explicit MyFilter(const YAML::Node& conf)
      : timeout_(std::chrono::milliseconds(conf["timeout_ms"].as<int64_t>())) {}

  bool Accept(const DepthData& depth) final {
    return depth.local_timestamp - depth.exchange_timestamp < timeout;
  }

 private:
  std::chrono::milliseconds timeout_;
};

REGISTER_MARKET_DATA_FILTER("my_filter", MyFilter);

}  // namespace xyts

```

```c
add_library(my_filter SHARED)
target_include_directories(
my_filter
  PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/third_party/yaml-cpp/include")
```

```yaml
# market_center.yaml
market_data_filters:
  - name: timeout_filter
    timeout_ms: 1000
```

## 实盘DataCollector扩展

交易过程中，交易信息（如订单、成交、持仓、账户资金、告警等）会通过发送到DataCollector模块用于实时监控，xyts默认提供了企业微信告警通知模块，用户也可自定义DataHandler来跟第三方或自己开发的监控进行对接。下面是个扩展的例子

```cpp
#pragma once

#include <openssl/tls1.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/system/error_code.hpp>

#include "xyts/data_collector/collected_data_handler.h"
#include "yaml-cpp/yaml.h"

namespace xyts {

class WechatSender final : public CollectedDataHandler {
 public:
  explicit WechatSender(const YAML::Node& conf);

  void Handle(CollectedDataType type, const std::string& data) final;

 private:
  void SendAlarmMsg(const std::string& data);

  boost::asio::io_context ioc_;
  boost::asio::ssl::context ssl_{boost::asio::ssl::context::tlsv12_client};
  std::string wechat_host_ = "qyapi.weixin.qq.com";
  std::string wechat_robot_key_;
  std::string target_;
};

}  // namespace xyts
```

## 实盘TradeApi扩展

xyts目前对接了以下api:

- ctp
- ctp2mini
- binance
- yd
- xele
- xtp

如需对接其他交易API，可继承TradeApi并实现相应的虚函数

```cpp
class TradeApi {
 public:
  virtual ~TradeApi() = default;

  virtual bool SendOrder(const OrderRequest& request) = 0;

  virtual bool CancelOrder(const CancellationRequest& request) = 0;

  virtual TradeInfo QueryTradeInfo() = 0;

  virtual std::vector<Contract> QueryContracts() = 0;

  virtual std::vector<Account> QueryAccounts() = 0;

  auto* GetEventRing() { return &event_ring_; }

 protected:
  template <class EventType>
  void PushEvent(const EventType& event) {
    auto* ptr = event_ring_.PrepareEnqueueBlocking();
    *ptr = event;
    event_ring_.CommitEnqueue();
  }

 private:
  using OrderEventRing = xyu::SPSCRingBuffer<OrderEvent, 1024>;
  OrderEventRing event_ring_;
};
```

## 实盘DataFeedApi扩展

xyts目前对接了以下api:

- ctp
- ctp2mini
- binance
- yd
- xtp

如需对接其他行情API，可继承DataFeedApi并实现相应的虚函数

```cpp
class DataFeedApi : public xyu::NonCopyableNonMoveable {
 public:
  virtual ~DataFeedApi() = default;

  virtual DataFeedStatus GetStatus() const = 0;

  virtual bool Subscribe(const std::vector<std::string>& patterns) = 0;

  virtual bool Unsubscribe(const std::vector<std::string>& patterns) = 0;
};
```

## OrderBook

实时合成OrderBook，且能够以自定义频率进行快照采样，目前支持的交易所有：上交所、深交所、港交所

## 回测里的MatchingEngine扩展

## 回测里的自定义费率

## 合约标准命名规则

### 中国期货

`FUT_{exchange}_{symbol}-{YYYYmm}`

其中exchange和symbol对应交易所的命名，如
- FUT_SHFE_rb-202405
- FUT_CZCE_WH-202405

包括郑商所在内的所有交易所都遵循上述6位到期日的命名规则

主力合约的YYYYmm规定为111111，如
- FUT_SHFE_rb-111111
- FUT_SHFE_hc-111111

### 期权

`OPT_{exchange}_{symbol}-{YYYYmm}-{C/P}-{strike}`

如
- OPT_XSHG_510300-202409-C-3.25
- OPT_CFFEX_IO-202409-P-3500

## xydata

xydata提供历史行情数据和当日历史行情数据的访问接口

xydata数据的组织方式

```sh
data/
    depth/
        2024-01-15/
            FUT_SHFE_rb-202405.csv
    contract/
        comm_deri_contracts_2024-01-15.csv
    holiday/
        2024.csv
```

除了holiday中的内容需要自己手动填写，合约和行情数据都能从实盘中获取并存储下来，XYTS也支持每天落地数据并更新到xydata中

### depth

快照按照日期及合约名来存储

以FUT_SHFE_rb-202405.csv前几行为例来说明数据格式

```csv
local_timestamp,exchange_timestamp,open_interest,volume,turnover,last_price,bid_volume_1,bid_price_1,ask_volume_3,ask_price_3,ask_volume_1,ask_price_1,bid_volume_4,bid_price_4,bid_volume_2,bid_price_2,ask_volume_4,ask_price_4,ask_volume_2,ask_price_2,bid_volume_5,bid_price_5,bid_volume_3,bid_price_3,ask_volume_5,ask_price_5,instrument
1705063808381581,1705055979200000,1518179,0,0,3902,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,FUT_SHFE_rb-202405
1705064340608710,1705064340500000,1520933,7550,294827500,3905,836,3905,230,3908,1036,3906,1185,3902,6,3904,283,3909,137,3907,43,3901,22,3903,477,3910,FUT_SHFE_rb-202405
1705064400607038,1705064400500000,1520956,8592,335515850,3904,13,3903,1029,3906,3,3904,1683,3900,1184,3902,160,3907,404,3905,216,3899,109,3901,280,3908,FUT_SHFE_rb-202405
1705064401083261,1705064401000000,1520861,9226,360268510,3905,10,3904,160,3907,366,3905,103,3901,124,3903,290,3908,1040,3906,1753,3900,1139,3902,305,3909,FUT_SHFE_rb-202405
1705064401638857,1705064401500000,1521087,9891,386227820,3904,231,3904,162,3907,349,3905,107,3901,5,3903,282,3908,1053,3906,1755,3900,1029,3902,437,3909,FUT_SHFE_rb-202405
```

数据字段，顺序无关紧要：

- local_timestamp: 本地收到数据的微秒时间戳
- exchange_timestamp: 交易所微秒时间戳
- open_interest: 持仓量
- volume: 成交量
- turnover: 成交额
- last_price: 最新成交价
- bid_volume_1 ~ bid_volume_5: 买1到买5的量
- bid_price_1 ~ bid_price_5: 买1到买5的价格
- ask_volume_1 ~ ask_volume_5: 卖1到卖5的量
- ask_price_1 ~ ask_price_5: 卖1到卖5的价格
- instrument: 合约标准命名


### contract

以comm_deri_contracts_2024-01-15.csv前几行为例来说明数据格式

```csv
contract_id,instr,code,exchange,product_type,contract_unit,price_tick,upper_limit_price,lower_limit_price,long_margin_rate,short_margin_rate,max_limit_order_volume,min_limit_order_volume,max_market_order_volume,min_market_order_volume,list_date,expire_date,underlying_type,underlying_symbol,exercise_date,exercise_price
1,FUT_SHFE_pb-202412,pb2412,SHFE,Futures,5,5,17290,15335,0.08,0.08,500,1,30,1,,2024-12-16,Unknown,,,0
2,FUT_SHFE_rb-202401,rb2401,SHFE,Futures,10,1,4123,3730,0.2,0.2,500,30,30,30,,2024-01-15,Unknown,,,0
3,FUT_SHFE_rb-202402,rb2402,SHFE,Futures,10,1,4008,3627,0.1,0.1,500,1,30,1,,2024-02-19,Unknown,,,0
4,FUT_SHFE_rb-202403,rb2403,SHFE,Futures,10,1,4066,3679,0.07,0.07,500,1,30,1,,2024-03-15,Unknown,,,0
5,FUT_SHFE_rb-202404,rb2404,SHFE,Futures,10,1,4085,3696,0.07,0.07,500,1,30,1,,2024-04-15,Unknown,,,0
6,FUT_SHFE_rb-202405,rb2405,SHFE,Futures,10,1,4110,3719,0.07,0.07,500,1,30,1,,2024-05-15,Unknown,,,0
```

数据字段，顺序无关紧要：

- contract_id: 从1开始连续递增，主要是为了人看的时候方便对应合约，实际上随便填也没关系
- instr: 标准合约名
- code: 合约在交易所的命名
- exchange: 交易所名
- product_type: 产品类型, Futures/Options/Stock/...
- contract_unit: 合约乘数
- price_tick: 最小价格变动单位
- upper_limit_price: 涨停价
- lower_limit_price: 跌停价
- long_margin_rate: 多头保证金率
- short_margin_rate: 空头保证金率
- max_limit_order_volume: 限价单一笔最大的量
- min_limit_order_volume: 限价单一笔最小的量
- max_market_order_volume: 市价单一笔最大的量
- min_market_order_volume: 市价单一笔最小的量
- list_date: 上市日
- expire_date: 到期日
- underlying_type: 标的的产品类型
- underlying_symbol: 标的的代码
- exercise_date: 如果是期权产品则表示行权日，否则留空
- exercise_price: 如果是期权产品则表示行权价，否则留空

### holiday

holiday中按年存储，每年一个csv，命名为YYYY.csv，里面存储的内容是当前的所有假期。

注意事项：
- 如果假期连着周末，则周末也视作法定节假日，这一做法主要是因为节假日或是节假日前一天或是连带的那个周五没有夜盘。
- 如果年底连着下一年的元旦假期，则年底的那几天算做当年的节假日

以2024.csv为例

```csv
holiday
2024-01-01
2024-02-10
2024-02-11
2024-02-12
2024-02-13
2024-02-14
2024-02-15
2024-02-16
2024-02-17
2024-04-04
2024-04-05
2024-04-06
2024-05-01
2024-05-02
2024-05-03
2024-05-04
2024-05-05
2024-06-08
2024-06-09
2024-06-10
2024-09-15
2024-09-16
2024-09-17
2024-10-01
2024-10-02
2024-10-03
2024-10-04
2024-10-05
2024-10-06
2024-10-07
```
