// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_UTILS_STRING_UTILS_H_
#define FT_INCLUDE_UTILS_STRING_UTILS_H_

#include <string>
#include <vector>

namespace ft {

inline void split(const std::string_view& str, const std::string_view& delim,
                  std::vector<std::string>* results) {
  std::size_t start = 0, end, size;
  while ((end = str.find(delim, start)) != std::string::npos) {
    size = end - start;
    if (size != 0)
      results->emplace_back(std::string(str.cbegin() + start, size));
    start = end + delim.size();
  }

  if (start != str.size())
    results->emplace_back(
        std::string(str.cbegin() + start, str.size() - start));
}

}  // namespace ft

#endif  // FT_INCLUDE_UTILS_STRING_UTILS_H_
