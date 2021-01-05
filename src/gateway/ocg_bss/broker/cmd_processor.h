// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_BROKER_CMD_PROCESSOR_H_
#define BSS_BROKER_CMD_PROCESSOR_H_

#include "broker/command.h"

namespace ft {
class BssBroker;
}

namespace ft::bss {

class CmdProcessor {
 public:
  CmdProcessor(::ft::BssBroker* broker);

  void start(int port);

 private:
  bool ProcessCmd(const CmdHeader& hdr, const char* body);
  void process_logon(const LogonCmd& cmd);
  void process_logout();
  void process_new_order(const NewOrderCmd& cmd);
  void process_amend_order(const AmendOrderCmd& cmd);
  void process_cancel_order(const CancelOrderCmd& cmd);
  void process_mass_cancel(const MassCancelCmd& cmd);

 private:
  ::ft::BssBroker* broker_;
};

}  // namespace ft::bss

#endif  // BSS_BROKER_CMD_PROCESSOR_H_
