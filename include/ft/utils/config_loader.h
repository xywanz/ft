// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_CONFIG_LOADER_H_
#define FT_INCLUDE_FT_UTILS_CONFIG_LOADER_H_

#include <string>

#include "ft/base/config.h"

namespace ft {

bool LoadConfig(const std::string& file, ft::Config* config);

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_CONFIG_LOADER_H_
