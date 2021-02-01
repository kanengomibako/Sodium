#ifndef LIB_OSC_HPP
#define LIB_OSC_HPP

#include "common.h"
#include "lib_calc.hpp"

// 三角波出力
class triangleWave
{
private:
  uint32_t count = 0, rateCount = 20000;

public:
  triangleWave()
  {
  }

  void set(float rate) // 周期（秒）設定
  {
    rateCount = rate * SAMPLING_FREQ;
  }

  void set(float rate, float phase) // 周期（秒）、位相（0～1）設定
  {
    rateCount = rate * SAMPLING_FREQ;
    count = (uint32_t)((float)rateCount * phase);
  }

  float output()
  {
    count++;
    if (count >= rateCount) count = 0; // 1周期で0に戻る

    float y = (float)count / (float)rateCount;
    if (count > rateCount / 2) y = 1.0f - y; // 周期の半分で折り返す
    return 2.0f * y; // 0 ... 1 Triangle Wave
  }

};

#endif // LIB_OSC_HPP
