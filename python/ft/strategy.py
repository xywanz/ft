import redis

import ft.order_sender as order_sender


class OrderResponse(object):
    def __init__(self, data):
        pass


class Tick(object):
    def __init__(self, data):
        pass


class Strategy(object):
    def __init__(self, strategy_id):
        self.strategy_id = strategy_id
        self.sender = order_sender.OrderSender(strategy_id=strategy_id)
        self.tick_redis = redis.StrictRedis(host='localhost', port=6379)
        self.tick_sub = self.tick_redis.pubsub()

    def on_init(self):
        pass

    def on_tick(self, tick):
        print('on_tick')

    def on_order_rsp(self, order):
        pass

    def on_exit(self):
        pass

    def subscribe(self, tickers):
        self.tick_sub.subscribe(tickers)

    def send_order(self, ticker, direction, offset, order_type, volume, price):
        self.sender.send_order(ticker, direction, offset,
                               order_type, volume, price)

    def run(self):
        self.on_init()

        if self.strategy_id:
            self.subscribe(Strategy)

        for reply in self.tick_sub.listen():
            if reply['channel'] == self.strategy_id:
                rsp = OrderResponse(reply['data'])
                self.on_order_rsp(rsp)
            else:
                tick = Tick(reply['data'])
                self.on_tick(tick)
