// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef CPPEX_INCLUDE_CPPEX_SPLIT_H_
#define CPPEX_INCLUDE_CPPEX_SPLIT_H_

#include <string>
#include <type_traits>
#include <vector>

template<class OutputString>
void split(
  const std::basic_string_view<typename OutputString::value_type>& str,
  const std::basic_string_view<typename OutputString::value_type>& delim,
  std::vector<OutputString>& results
) {
  std::size_t start = 0, end, size;
  while ((end = str.find(delim, start)) != std::string::npos) {
    size = end - start;
    if (size != 0)
      results.push_back(OutputString(str.cbegin() + start, size));
    start = end + delim.size();
  }

  if (start != str.size())
    results.push_back(OutputString(str.cbegin() + start, str.size() - start));
}

template<class OutputString>
std::vector<OutputString> split(
  const std::basic_string_view<typename OutputString::value_type>& str,
  const std::basic_string_view<typename OutputString::value_type>& delim
) {
  std::vector<OutputString> results;
  split(str, delim, results);
  return results;
}

#endif  // CPPEX_INCLUDE_CPPEX_SPLIT_H_
