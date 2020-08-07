// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef BSS_BROKER_COMMAND_H_
#define BSS_BROKER_COMMAND_H_

#include "protocol/protocol.h"

namespace ft::bss {

inline uint32_t kCommandMagic = 0x9394;

enum CmdType {
  BSS_CMD_LOGON = 100,
  BSS_CMD_LOGOUT,
  BSS_CMD_NEW_ORDER,
  BSS_CMD_AMEND_ORDER,
  BSS_CMD_CANCEL_ORDER,
  BSS_CMD_MASS_CANCEL,
};

struct LogonCmd {
  char password[9];
  char new_password[9];
} __attribute__((__packed__));

struct NewOrderCmd {
} __attribute__((__packed__));

struct AmendOrderCmd {
} __attribute__((__packed__));

struct CancelOrderCmd {
} __attribute__((__packed__));

struct MassCancelCmd {
} __attribute__((__packed__));

struct CmdHeader {
  uint32_t magic;
  uint32_t type;
  uint32_t length;
  uint32_t reserved;
} __attribute__((__packed__));

}  // namespace ft::bss

#endif  // BSS_BROKER_COMMAND_H_
