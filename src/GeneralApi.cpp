// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "GeneralApi.h"

#include <functional>
#include <map>

#include "Api/Ctp/CtpApi.h"

namespace ft {

GeneralApi* create_api(const std::string& name, EventEngine* engine) {
  if (name == "ctp")
    return new CtpApi(engine);
  else
    return nullptr;
}

void destroy_api(GeneralApi* api) {
  if (api)
    delete api;
}

}  // namespace ft
