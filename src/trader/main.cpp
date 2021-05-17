// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "ft/base/contract_table.h"
#include "getopt.hpp"
#include "spdlog/spdlog.h"
#include "trader/oms.h"

static void Usage(const char* pname) {
  printf("Usage: %s [--config=<file>] [-h -? --help] [--loglevel=level]\n", pname);
  printf("    --config            登录的配置文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --loglevel          日志等级(trace, debug, info, warn, error)\n");
}

int main(int argc, char** argv) {
  std::string config_file = getarg("../config/config.yml", "--config");
  std::string log_level = getarg("info", "--loglevel");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    Usage(argv[0]);
    exit(EXIT_SUCCESS);
  }

  spdlog::set_level(spdlog::level::from_str(log_level));

  ft::FlareTraderConfig config;
  if (!config.Load(config_file)) {
    spdlog::error("failed to load config from {}", config_file);
    exit(EXIT_FAILURE);
  }

  auto oms = std::make_unique<ft::OrderManagementSystem>();
  if (!oms->Init(config)) {
    spdlog::error("failed to init oms");
    exit(EXIT_FAILURE);
  }

  oms->ProcessCmd();
}
