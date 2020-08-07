// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <spdlog/spdlog.h>

#include <getopt.hpp>

#include "cep/data/contract_table.h"
#include "cep/oms/config_loader.h"
#include "cep/oms/oms.h"

ft::OMS* oms = nullptr;

static void usage() {
  printf("usage: ./trading-engine [--config=<file>] [--contracts=<file>]\n");
  printf("                        [-h -? --help] [--loglevel=level]\n");
  printf("\n");
  printf("    --config            登录的配置文件\n");
  printf("    --contracts         合约列表文件\n");
  printf("    -h, -?, --help      帮助\n");
  printf("    --loglevel          日志等级(info, warn, error, debug, trace)\n");
}

int main() {
  std::string login_config_file =
      getarg("../config/ctp_config.yml", "--config");
  std::string log_level = getarg("info", "--loglevel");
  bool help = getarg(false, "-h", "--help", "-?");

  if (help) {
    usage();
    exit(0);
  }

  spdlog::set_level(spdlog::level::from_str(log_level));

  ft::Config config;
  ft::load_config(login_config_file, &config);

  oms = new ft::OMS;
  if (!oms->login(config)) exit(-1);

  oms->process_cmd();
}
