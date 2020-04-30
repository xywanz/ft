// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <iostream>
#include <string>
#include <string_view>

#include "cppex/split.h"

template<class S1, class S2>
void test_split(S1 &&str, S2 &&delim) {
  std::vector<std::string> v;
  split(std::forward<S1>(str), std::forward<S2>(delim), v);
  std::cout << "|";
  for (auto s : v)
    std::cout << s << "|";
  std::cout << std::endl;
}

int main() {
  test_split("", ",");
  test_split(",", ",");
  test_split("hello", ",");
  test_split("hello,", ",");
  test_split(",hello", ",");
  test_split("hello, world", ",");
  test_split("hello,", ",");
  test_split("hello,,", ",");
  test_split("hello, ,", ",");
}
