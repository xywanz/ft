// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "GeneralApi.h"

#include <functional>
#include <map>

#include "Api/Ctp/CtpApi.h"

namespace ft {

std::map<std::string, __API_CREATE_FUNC>& __get_api_map() {
  static std::map<std::string, __API_CREATE_FUNC> type_map;

  return type_map;
}

GeneralApi* create_api(const std::string& name, EventEngine* engine) {
  auto& type_map = __get_api_map();
  auto iter = type_map.find(name);
  if (iter == type_map.end())
    return nullptr;
  return iter->second(engine);
}

void destroy_api(GeneralApi* api) {
  if (api)
    delete api;
}

}  // namespace ft
