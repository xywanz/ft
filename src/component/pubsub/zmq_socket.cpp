// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "component/pubsub/zmq_socket.h"

#include <spdlog/spdlog.h>

#include <vector>

namespace ft::pubsub {

bool ZmqSocket::Connect(const std::string& address) {
  try {
    zmq_ctx_ = std::make_unique<zmq::context_t>();
    zmq_sock_ = std::make_unique<zmq::socket_t>(*zmq_ctx_, zmq::socket_type::sub);
    zmq_sock_->connect(address.c_str());
  } catch (...) {
    return false;
  }

  return true;
}

bool ZmqSocket::Bind(const std::string& address) {
  try {
    zmq_ctx_ = std::make_unique<zmq::context_t>();
    zmq_sock_ = std::make_unique<zmq::socket_t>(*zmq_ctx_, zmq::socket_type::pub);
    zmq_sock_->bind(address.c_str());
  } catch (...) {
    return false;
  }

  return true;
}

bool ZmqSocket::Subscribe(const std::string& topic) {
  zmq_sock_->set(zmq::sockopt::subscribe, topic.c_str());
  return true;
}

bool ZmqSocket::SendMsg(const std::string& topic, const std::string& msg) {
  try {
    zmq_sock_->send(zmq::buffer(topic.data(), topic.size()), zmq::send_flags::sndmore);
    zmq_sock_->send(zmq::buffer(msg.data(), msg.size()));
  } catch (...) {
    return false;
  }

  return true;
}

bool ZmqSocket::RecvMsg(std::string* topic, std::string* msg) {
  std::vector<zmq::message_t> recv_msgs;
  zmq::recv_result_t result = zmq::recv_multipart(*zmq_sock_, std::back_inserter(recv_msgs));
  if (!result || *result != 2) {
    spdlog::error("zmq::recv_multipart() failed");
    return false;
  }
  if (topic) {
    *topic = recv_msgs[0].to_string();
  }
  if (msg) {
    *msg = recv_msgs[1].to_string();
  }

  return true;
}

}  // namespace ft::pubsub
