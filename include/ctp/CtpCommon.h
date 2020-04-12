// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_CTP_CTPCOMMON_H_
#define FT_INCLUDE_CTP_CTPCOMMON_H_

#include <codecvt>
#include <locale>
#include <string>
#include <vector>

#include <ThostFtdcUserApiDataType.h>

namespace ft {

inline bool is_error_rsp(CThostFtdcRspInfoField *rsp_info) {
  if (rsp_info && rsp_info->ErrorID != 0)
    return true;
  return false;
}

inline std::string gb2312_to_utf8(const std::string &gb2312)
{
  static const std::locale loc("zh_CN.GB18030");

  std::vector<wchar_t> wstr(gb2312.size());
  wchar_t* wstr_end = nullptr;
  const char* gb_end = nullptr;
  mbstate_t state {};
  int res = std::use_facet<std::codecvt<wchar_t, char, mbstate_t>>(loc).in(
          state,
          gb2312.data(), gb2312.data() + gb2312.size(), gb_end,
          wstr.data(), wstr.data() + wstr.size(), wstr_end);

  if (res == std::codecvt_base::ok) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> cutf8;
      return cutf8.to_bytes(std::wstring(wstr.data(), wstr_end));
  }

  return "";
}

}  // namespace ft

#endif  // FT_INCLUDE_CTP_CTPCOMMON_H_
