# Gateway开发文档
Gateway用于适配不同的交易行情API或是协议，开发者通过继承Gateway基类，并重写各个接口以完成对接。
## 1. 接口说明
### 登录
```cpp
virtual bool Gateway::Login(BaseOrderManagementSystem* oms, const Config& config);
```
* OMS通过调用Gateway的Login函数来登录到交易及行情服务器，所以Login里面需要实现完整的登录逻辑，包括交易及行情，并进行一些初始化的工作。
* OMS会将自身的指针传递给Gateway，Gateway需要把oms指针保存下来，并在收到交易回报及行情时，调用oms相应的回调函数。
* Config是登录相关的配置参数，如账号密码等信息，如果Gateway需要额外的独有的参数，可以通过扩展的参数arg0-arg8来配置，或是在Config中新增配置项。
* 该接口为同步的，登录成功且能接收行情、能够交易后才返回true，如果API（如CTP）登录过程也涉及异步回调的，需要开发者自行做同步操作。

使用示例
```cpp
class MyGateway : public Gateway {
 public:
  bool Login(BaseOrderManagementSystem* oms, const Config& config) override {
    oms_ = oms;
    // login
    return true;
  }

 private:
  BaseOrderManagementSystem* oms_;
};
```

### 登出
```cpp
virtual void Gateway::Logout();
```
* OMS通过调用Gateway的Logout函数来从交易及行情服务器登出，所以Logout里需要实现完整的登出逻辑，以及可能需要进行资源释放的操作
* 该接口是同步的，当完全登出时才可以返回。如果API（如CTP）登出过程也涉及异步回调的，需要开发者自行做同步操作
* 如果没有登出需求的，可以不实现该接口

### 订单发送
```cpp
virtual bool Gateway::SendOrder(const OrderRequest& order, uint64_t* privdata_ptr);
```
* OMS通过调用Gateway的SendOrder函数来发送订单
* 需要注意的是，订单请求中包含有一个order_id字段，该字段是OMS生成的一个订单ID，这个ID会从1开始不断递增1。order_id主要用于区分回报消息是属于哪个订单的，当Gateway收到交易回报时，需要将order_id也告知OMS
* privdata_ptr参数是用于撤单的，Gateway可以通privdata_ptr向Gateway传递一个64位的数值。当OMS调用Gateway的撤单函数撤销某个order_id的订单时，OMS会将该order_id所对于的privdata的值传回给Gateway。

### 撤单
```cpp
virtual bool Gateway::CancelOrder(uint64_t order_id, uint64_t privdata);
```
* OMS通过调用Gateway的CancelOrder函数来撤销订单
* privdata是SendOrder时Gateway自行设置的

### 行情订阅
```cpp
virtual bool Gateway::Subscribe(const std::vector<std::string>& sub_list);
```
* OMS通过调用Gateway的Subscribe函数来订阅行情
* 订阅列表示例：{"rb2105", "rb2106"}，注意合约名称的大小写，不同交易所大小写规则不同
* 该接口无需等待订阅成功，可在调用API的订阅接口后立即返回

### 合约查询
```cpp
struct Contract {
  std::string ticker;
  std::string exchange;
  std::string name;
  ProductType product_type;
  int size;
  double price_tick;

  double long_margin_rate;
  double short_margin_rate;

  int max_market_order_volume;
  int min_market_order_volume;
  int max_limit_order_volume;
  int min_limit_order_volume;

  int delivery_year;
  int delivery_month;

  uint32_t ticker_id;  // local index
};

virtual bool Gateway::QueryContract(const std::string& ticker, const std::string& exchange, Contract* result);
virtual bool Gateway::QueryContractList(std::vector<Contract>* result);
```
* OMS通过调用Gateway的QueryContract函数来查询单个合约信息，通过调用QueryContractList来查询全市场合约信息
* 该接口为同步接口，需要把合约信息存入相应的地址或是vector里

### 仓位查询
```cpp
struct PositionDetail {
  int yd_holdings;
  int holdings;
  int frozen;
  int open_pending;
  int close_pending;

  double cost_price;
  double float_pnl;
};

struct Position {
  uint32_t ticker_id;
  PositionDetail long_pos;
  PositionDetail short_pos;
};

virtual bool Gateway::QueryPosition(const std::string& ticker, Position* result);
virtual bool Gateway::QueryPositionList(std::vector<Position>* result)；
```
* OMS通过调用Gateway的QueryPosition函数来查询单个品种的持仓信息，通过调用QueryPositionList来查询所有持仓信息
* 该接口为同步接口，需要把仓位信息存入相应的地址或是vector里
* Position结构体里保存了多空两个方向的信息

### 资金查询
```cpp
struct Account {
  uint64_t account_id;  // 资金账户号
  double total_asset;   // 总资产
  double cash;          // 可用资金
  double margin;        // 保证金
  double frozen;        // 冻结金额
}

virtual bool Gateway::QueryAccount(Account* result);
```
* OMS通过调用Gateway的QueryAccount函数来查询账户资金信息
* 该接口为同步接口，需要把账户信息写入指定的地址

## 2. 向OMS推送回报及行情
Gateway需要自己实现API需要的回调函数，并在回调函数中，调用oms的相应接口，将该消息转发给oms

### 订单被接受
```cpp
struct OrderAcceptance {
  uint64_t order_id;
};

void BaseOrderManagementSystem::OnOrderAccepted(OrderAcceptance* rsp);
```
* Gateway收到前置机或是交易所发来的订单被接收（并没有发生成交）的消息后，调用oms的该接口，通知oms订单已被接收
* 如果第一个订单回报到来时就已经发生了部分或全部成交，那么无需调用此函数

### 订单被拒
```cpp
struct OrderRejection {
  uint64_t order_id;
  std::string reason;
};

void BaseOrderManagementSystem::OnOrderRejected(OrderRejection* rsp);
```
* Gateway收到订单被拒的消息后，调用oms的该接口，通知oms订单被拒

### 成交回报
```cpp
struct Trade {
  uint64_t order_id;
  uint32_t ticker_id;
  Direction direction;
  Offset offset;
  TradeType trade_type;
  int volume;
  double price;
  double amount;  // 本次交易金额

  datetime::Datetime trade_time;
};

void BaseOrderManagementSystem::OnOrderTraded(Trade* rsp);
```
* Gateway收到成交回报消息后，调用oms的该接口，通知oms发生了成交
* 对于二级市场成交，如股票买卖、期货多空等，trade_type必须为TradeType::kSecondaryMarket

### 撤单成功回报
```cpp
struct OrderCancellation {
  uint64_t order_id;
  int canceled_volume;
};

void BaseOrderManagementSystem::OnOrderCanceled(OrderCancellation* rsp);
```
* Gateway收到撤单成功回报时，调用oms的该接口，通知oms撤单成功

### 撤单被拒回报
```cpp
struct OrderCancelRejection {
  uint64_t order_id;
  std::string reason;
};

void BaseOrderManagementSystem::OnOrderCancelRejected(OrderCancelRejection* rsp);
```
* Gateway收到撤单失败回报时，调用oms的该接口，通知oms撤单失败
* 该接口可以不实现

## 3. 注册Gateway
在你的gateway源代码文件中，注册你的Gateway，注意，要在namespace外进行注册。假如你的Gateway源码文件为my_gateway.cpp，内容如下
```cpp
namespace ft {
class MyGateway
}  // namespace ft

REGISTER_GATEWAY(::ft::MyGateway);  // 注册Gateway
```

## 4. 项目路径及CMake配置
假设你要开发的gateway叫做my_gateway，按以下步骤进行
1. 在ft/src/gateway下创建my_gateway目录，即ft/src/gateway/my_gateway
2. 在ft/src/gateway/my_gateway目录下添加你的代码文件，假设为my_gateway.h my_gateway.cpp
3. 在ft/src/gateway/my_gateway目录下创建CMakeLists.txt
```cmake

add_library(my_gateway SHARED my_gateway.cpp)
add_library(ft::my_gateway ALIAS my_gateway)

#target_include_directories(my_gateway PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/third_party/my_gateway/include")
#target_link_directories(my_gateway PRIVATE "${PROJECT_SOURCE_DIR}/third_party/my_gateway/lib")
target_link_libraries(my_gateway PRIVATE
    ft::ft_header
    ft::base
    ft::component
    ft::utils
    ft::ft_third_party
#    my_gateway_trade_api
#    my_gateway_md_api
    pthread)

```

4. 在ft/src/gateway/CMakeLists.txt中添加my_gateway路径
```cmake
add_subdirectory(my_gateway)
```

## 5. Q&A
Q: SendOrder中的order_id如何与回报中的order_id对应上  
A: 参考CTP的实现，通过在order_ref和order_id之间建立某种映射关系，例如order_ref = order_id + order_ref_base，从而可以从order_ref反推出order_id。而每个回报中都带有order_ref信息，所以也就知道了order_id
