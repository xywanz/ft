// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_INCLUDE_FT_UTILS_MISC_H_
#define FT_INCLUDE_FT_UTILS_MISC_H_

#define UNUSED(x) ((void)(x))

namespace ft {

// 判断浮点数是否相等
template <class RealType>
bool IsEqual(const RealType& lhs, const RealType& rhs, RealType error = RealType(1e-5)) {
  return rhs - error <= lhs && lhs <= rhs + error;
}

}  // namespace ft

#endif  // FT_INCLUDE_FT_UTILS_MISC_H_
