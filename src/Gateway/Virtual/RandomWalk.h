// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef FT_SRC_GATEWAY_VIRTUAL_RANDOMWALK_H_
#define FT_SRC_GATEWAY_VIRTUAL_RANDOMWALK_H_

#include <random>

namespace ft {

class RandomWalk {
 public:
  RandomWalk(double start, double step);

  double next();

 private:
  double start_;
  double step_;
  double current_;

  std::default_random_engine engine_;
  std::uniform_real_distribution<double> dist_{0, 1};
};

}  // namespace ft

#endif  // FT_SRC_GATEWAY_VIRTUAL_RANDOMWALK_H_
