# Flare Trader 算法交易系统
| Platform | Build                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------- |
| Linux    | [![Build Status](https://travis-ci.com/DuckDuckDuck0/ft.svg?branch=master)](https://travis-ci.com/DuckDuckDuck0/ft) |

Flare Trader是一套量化交易解决方案，旨在追求以下目标：
* 极低的交易时延，框架耗时控制在2us以内 (进行中)
* 提供从策略开发到部署落地的整个完整的解决方案
* 友好的策略开发接口及框架
* 模块化的设计，可以对各个组件进行定制化开发

目前Flare Trader离上面所列举的目标还差很远，但会持续地更新，大家有想法或是发现了bug，可以在issue上面自由讨论，也欢迎大家参与到ft的开发讨论中。

详细使用及开发文档请移步wiki页面
https://github.com/DuckDuckDuck0/ft/wiki

QQ交流群：341031341

Table of Contents
=================





   * [Flare Trader 算法交易系统](#flare-trader-算法交易系统)
   * [目录](#目录)
      * [Backgroud](#backgroud)
      * [Install](#install)
      * [Usage](#usage)
      * [Maintainers](#maintainers)

## Backgroud
为了解决当前众多Python量化交易框架性能低的问题

先上张图快速了解下ft的架构
![framework](img/framework.png)

## Install
使用cmake(>=3.13)及g++(>=7)进行编译
```bash
$ git clone https://github.com/DuckDuckDuck0/ft.git
$ cd ft
$ mkdir build && cd build
$ cmake ..
$ make -j
```

## Usage
以进行ctp交易为例，假设你现在处于build目录中

先进行ctp登录信息的配置
```bash
$ vim ../config/ctp_config.yml
```
下面是一份simnow的配置文件
```yaml
api: ./libctp_gateway.so
trade_server_address: tcp://180.168.146.187:10130
quote_server_address: tcp://180.168.146.187:10131
broker_id: 9999
investor_id: 123456  # 你的simnow账户
password: abc123     # 你的simnow密码
auth_code: 0000000000000000
app_id: simnow_client_test
subscription_list: [rb2104,rb2105]  # 订阅rb2104,rb2105两个品种的行情

contracts_file: ../config/contracts.csv  # 合约文件

no_receipt_mode: false  # 无回报模式，开启此选项则策略不会收到回报
cancel_outstanding_orders_on_startup: true  # 启动时撤销所有未完成订单

# 流控配置，填0表示关闭该功能
throttle_rate_limit_period_ms: 0
throttle_rate_order_limit: 0
throttle_rate_volume_limit: 0
```
配置完成后，准备启动ft_trader交易通道

如果当前合约文件过于老旧，则在启动ft_trader前，使用contract_collect进行更新
```bash
$ ./contract_collector --config=../config/ctp_config.yml --output=../config/contracts.csv
```

在启动ft_trader前，先启动redis-server服务。如未安装，在ubuntu系统上可通过apt进行安装
```bash
$ sudo apt install redis-server
```
安装完毕后启动redis-server
```bash
$ redis-server
```

然后使用该配置文件启动ft_trader交易通道
```bash
$ ./ft_trader --config=../config/ctp_config.yml
```

最后启动demo策略
```bash
$ ./strategy_engine --account=123456 --contracts=../config/contracts.csv --id=spread_arb_0 --strategy=libspread_arb.so
```

## Maintainers
@DuckDuckDuck0
