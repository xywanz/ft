// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_LOGINPARAMS_H_
#define FT_INCLUDE_LOGINPARAMS_H_

#include <string>
#include <vector>

namespace ft {

class LoginParams {
 public:
  const std::string& front_addr() const {
    return front_addr_;
  }

  void set_front_addr(const std::string& front_addr) {
    front_addr_ = front_addr;
  }

  const std::string& md_server_addr() const {
    return md_server_addr_;
  }

  void set_md_server_addr(const std::string& md_server_addr) {
    md_server_addr_ = md_server_addr;
  }

  const std::string& broker_id() const {
    return broker_id_;
  }

  void set_broker_id(const std::string& broker_id) {
    broker_id_ = broker_id;
  }

  const std::string& investor_id() const {
    return investor_id_;
  }

  void set_investor_id(const std::string& investor_id) {
    investor_id_ = investor_id;
  }

  const std::string& passwd() const {
    return passwd_;
  }

  void set_passwd(const std::string& passwd) {
    passwd_ = passwd;
  }

  const std::string& auth_code() const {
    return auth_code_;
  }

  void set_auth_code(const std::string& auth_code) {
    auth_code_ = auth_code;
  }

  const std::string& app_id() const {
    return app_id_;
  }

  void set_app_id(const std::string& app_id) {
    app_id_ = app_id;
  }

  void set_subscribed_list(const std::vector<std::string>& list) {
    subscribed_list_ = list;
  }

  const auto& subscribed_list() const {
    return subscribed_list_;
  }

 private:
  std::string front_addr_;
  std::string md_server_addr_;
  std::string broker_id_;
  std::string investor_id_;
  std::string passwd_;
  std::string auth_code_;
  std::string app_id_;

  std::vector<std::string> subscribed_list_;
};

}  // namespace ft

#endif  // FT_INCLUDE_LOGINPARAMS_H_
