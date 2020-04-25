#ifndef FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_
#define FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_

namespace ft {

class IndicatorInterface {
 public:
  virtual void on_tick();
};

}  // namespace ft



#endif  // FT_INCLUDE_INDICATOR_INDICATORINTERFACE_H_
