// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <iostream>

#include "cppex/Any.h"

int main() {
  int* i = new int{1};
  const cppex::Any any = i;

  const int* ptr = any.cast<int>();
  std::cout << *ptr << std::endl;
}
