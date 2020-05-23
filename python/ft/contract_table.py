
class Contract(object):
    def __init__(self, ticker_index, ticker, exchange, name, size, price_tick):
        self.ticker_index = ticker_index
        self.ticker = ticker
        self.exchange = exchange
        self.name = name
        self.size = size
        self.price_tick = price_tick


class ContractTable(object):
    def __init__(self, file):
        self.contracts = []
        self.contract_map = {}
        self.contracts.append(None)
        i = 1
        with open(file) as f:
            f.readline()
            for line in f:
                fields = line.split(',')
                contract = Contract(i, fields[0], fields[1], fields[2],
                                    int(fields[4]), float(fields[5]))
                self.contracts.append(contract)
                i = i + 1
        for contract in self.contracts:
            if contract:
                self.contract_map[contract.ticker] = contract

    def get_by_index(self, i):
        if i > len(self.contracts):
            return None
        return self.contracts[i]

    def get_by_ticker(self, ticker):
        return self.contract_map[ticker]


ct = ContractTable('../config/contracts.csv')
