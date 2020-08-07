// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

// #include <thread>

// #include "broker/broker.h"
// #include "broker/cmd_processor.h"
// #include "broker/connection_manager.h"
// #include "broker/session.h"
// #include "broker/session_config.h"

// using namespace ft;
// using namespace ft::bss;

// int main() {
//   Broker broker;
//   Session session(&broker);
//   SessionConfig sess_conf{};
//   /* for test */
//   {
//     sess_conf.comp_id = "CO99999902";
//     sess_conf.password = "123aA678";
//     sess_conf.new_password = "2bB56789";
//     sess_conf.rsa_pubkey_file = "./5-RSA_public_key.pem";
//     sess_conf.msg_limit_per_sec = 8;
//   }

//   session.init(sess_conf);
//   session.enable();

//   std::thread([&broker] {
//     CmdProcessor cmd_processor(&broker);
//     cmd_processor.start(18889);
//   }).detach();

//   ConnectionManager conn_mgr(&session);
//   conn_mgr.run();
// }
