// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <ft/base/contract_table.h>
#include <ft/utils/config_loader.h>
#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "trader/order_management/order_management.h"

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
    exit(0);
  }

  spdlog::set_level(spdlog::level::from_str(log_level));

  ft::Config config;
  if (!ft::LoadConfig(config_file, &config)) {
    spdlog::error("FAILED to load config from {}", config_file);
  }

  auto oms = std::make_unique<ft::OrderManagementSystem>();
  if (!oms->Login(config)) {
    spdlog::error("FAILED to login");
    exit(-1);
  }

  spdlog::info("Successfully login. Ready to process commands");
  oms->ProcessCmd();
}
