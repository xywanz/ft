// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include "random_walk.h"

namespace ft {

RandomWalk::RandomWalk(double start, double step)
    : start_(start), step_(step), current_(start) {}

double RandomWalk::next() {
  double ret = current_;

  double up_or_down = dist_(engine_);
  if (up_or_down >= 0.5)
    current_ += step_;
  else
    current_ -= step_;

  return ret;
}

}  // namespace ft
