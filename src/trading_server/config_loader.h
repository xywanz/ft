// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_TRADING_SERVER_CONFIG_LOADER_H_
#define FT_SRC_TRADING_SERVER_CONFIG_LOADER_H_

#include <yaml-cpp/yaml.h>

#include <cassert>
#include <fstream>
#include <string>
#include <vector>

#include "trading_server/datastruct/config.h"
#include "utils/string_utils.h"

namespace ft {

void LoadConfig(const std::string& file, ft::Config* config);

}  // namespace ft

#endif  // FT_SRC_TRADING_SERVER_CONFIG_LOADER_H_
