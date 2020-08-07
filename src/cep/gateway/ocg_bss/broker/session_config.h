// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_SESSIONCONFIG_H
#define OCG_BSS_SESSIONCONFIG_H

#include <string>

namespace ft::bss {

struct SessionConfig {
  std::string comp_id;
  std::string password;
  std::string new_password;
  std::string rsa_pubkey_file;

  uint32_t msg_limit_per_sec;
};

}  // namespace ft::bss

#endif  // OCG_BSS_SESSIONCONFIG_H
