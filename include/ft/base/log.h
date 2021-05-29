// Copyright [2020-2021] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_BASE_LOG_H_
#define FT_INCLUDE_FT_BASE_LOG_H_

#include <memory>

#include "spdlog/spdlog.h"

namespace ft {

class Logger {
 public:
  static Logger& Instance() {
    static Logger logger;
    return logger;
  }

  std::shared_ptr<spdlog::logger> GetLogger() { return logger_; }

 private:
  Logger() : logger_(spdlog::default_logger()) {}

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

#define LOG_SET_LEVEL(log_level) \
  ::ft::Logger::Instance().GetLogger()->set_level(spdlog::level::from_str(log_level))

#define LOG_TRACE(...) ::ft::Logger::Instance().GetLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::ft::Logger::Instance().GetLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...) ::ft::Logger::Instance().GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::ft::Logger::Instance().GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::ft::Logger::Instance().GetLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...) ::ft::Logger::Instance().GetLogger()->critical(__VA_ARGS__)

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_LOG_H_
