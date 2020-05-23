import redis
import struct

import ft.constants as constants
import ft.contract_table as contract_table


def gen_order_req(ticker, direction, offset, volume, price,
                  order_type=constants.FAK, user_order_id=0,
                  strategy_id=''):
    contract = contract_table.ct.get_by_ticker(ticker)
    if not contract:
        return None

    req = struct.pack('<II16sIIIIIid', constants.CMD_MAGIC, constants.NEW_ORDER,
                      strategy_id.encode(encoding='utf-8'), user_order_id,
                      contract.ticker_index, direction, offset, order_type,
                      volume, price)

    return req


class OrderSender(object):
    def __init__(self, strategy_id='', host='localhost', port=6379):
        self.strategy_id = strategy_id
        self.redis = redis.StrictRedis(host, port)

    def send_order(self, ticker, direction, offset, order_type, volume, price,
                   user_order_id=0):
        req = gen_order_req(ticker, direction, offset, volume, price,
                            order_type, user_order_id, self.strategy_id)
        if req:
            self.redis.publish(constants.CMD_TOPIC, req)
            return True
        return False

    def cancel_order(self, order_id):
        pass
