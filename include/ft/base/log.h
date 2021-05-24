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

  spdlog::logger* GetLogger() { return logger_.get(); }

 private:
  Logger() : logger_(spdlog::default_logger()) {}

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(::ft::Logger::Instance().GetLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(::ft::Logger::Instance().GetLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(::ft::Logger::Instance().GetLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(::ft::Logger::Instance().GetLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(::ft::Logger::Instance().GetLogger(), __VA_ARGS__)
#define LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(::ft::Logger::Instance().GetLogger(), __VA_ARGS__)

}  // namespace ft

#endif  // FT_INCLUDE_FT_BASE_LOG_H_
