// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "cppex/split.h"

#include <string>
#include <string_view>

template
void split<std::string_view>(
  const std::string_view& str,
  const std::string_view& delim,
  std::vector<std::string_view>& results
);

template
void split<std::string>(
  const std::string_view& str,
  const std::string_view& delim,
  std::vector<std::string>& results
);

template
void split<std::basic_string_view<char16_t>>(
  const std::basic_string_view<char16_t>& str,
  const std::basic_string_view<char16_t>& delim,
  std::vector<std::basic_string_view<char16_t>>& results
);

template
void split<std::basic_string<char16_t>>(
  const std::basic_string_view<char16_t>& str,
  const std::basic_string_view<char16_t>& delim,
  std::vector<std::basic_string<char16_t>>& results
);

template
void split<std::basic_string_view<char32_t>>(
  const std::basic_string_view<char32_t>& str,
  const std::basic_string_view<char32_t>& delim,
  std::vector<std::basic_string_view<char32_t>>& results
);

template
void split<std::basic_string<char32_t>>(
  const std::basic_string_view<char32_t>& str,
  const std::basic_string_view<char32_t>& delim,
  std::vector<std::basic_string<char32_t>>& results
);

template
std::vector<std::string_view> split<std::string_view>(
  const std::string_view& str,
  const std::string_view& delim
);

template
std::vector<std::string> split<std::string>(
  const std::string_view& str,
  const std::string_view& delim
);

template
std::vector<std::basic_string_view<char16_t>>
split<std::basic_string_view<char16_t>>(
  const std::basic_string_view<char16_t>& str,
  const std::basic_string_view<char16_t>& delim
);

template
std::vector<std::basic_string<char16_t>>
split<std::basic_string<char16_t>>(
  const std::basic_string_view<char16_t> &str,
  const std::basic_string_view<char16_t> &delim
);

template
std::vector<std::basic_string_view<char32_t>>
split<std::basic_string_view<char32_t>>(
  const std::basic_string_view<char32_t> &str,
  const std::basic_string_view<char32_t> &delim
);

template
std::vector<std::basic_string<char32_t>>
split<std::basic_string<char32_t>>(
  const std::basic_string_view<char32_t> &str,
  const std::basic_string_view<char32_t> &delim
);
