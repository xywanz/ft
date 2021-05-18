// // Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

// #include "ft/strategy/order_sender.h"
// #include "getopt.hpp"

// static void Usage() {
//   printf("Usage:\n");
//   printf("    --account           账户\n");
//   printf("    -h, -?, --help      帮助\n");
//   printf("    --order_id          订单号\n");
//   printf("    --ticker            ticker or 'all'\n");
// }

// int main() {
//   std::string ticker = getarg("", "--ticker");
//   uint64_t account = getarg(0ULL, "--account");
//   uint64_t order_id = getarg(0ULL, "--order_id");
//   bool help = getarg(false, "-h", "--help", "-?");

//   if (help) {
//     Usage();
//     exit(0);
//   }

//   if (account == 0) {
//     printf("Invalid account\n");
//     exit(-1);
//   }

//   ft::OrderSender sender;
//   sender.SetAccount(account);

//   if (order_id != 0)
//     sender.CancelOrder(order_id);
//   else if (ticker == "all")
//     sender.CancelAll();
//   else
//     sender.CancelForTicker(ticker);
// }
