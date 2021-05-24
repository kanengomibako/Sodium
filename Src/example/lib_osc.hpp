#ifndef LIB_OSC_HPP
#define LIB_OSC_HPP

#include "common.h"
#include "lib_calc.hpp"

// のこぎり波
class sawWave
{
private:
  float y1 = 0.0f, freq = 0.1f; // 前回出力値、周波数

public:
  void set(float f) // 周波数設定
  {
    freq = f;
  }

  void set(float f, float phase)  // 周波数、位相（0～1）設定
  {
    freq = f;
    y1 = phase;
  }

  float output() // のこぎり波出力 0～1
  {
    float y = y1 + freq / SAMPLING_FREQ;
    if (y > 1) y = y - 1.0f;
    y1 = y; // 前回出力値を記録
    return y;
  }

};

// 正弦波
class sineWave
{
private:
  sawWave saw;

public:
  void set(float f) // 周波数設定
  {
    saw.set(f);
  }

  void set(float f, float phase)  // 周波数、位相（0～1）設定
  {
    saw.set(f, phase);
  }

  float output() // 正弦波出力 -1～1
  {
    return sinf(2.0f * PI * saw.output());
  }

};

// 三角波
class triangleWave
{
private:
  sawWave saw;

public:
  void set(float f) // 周波数設定
  {
    saw.set(f);
  }

  void set(float f, float phase)  // 周波数、位相（0～1）設定
  {
    saw.set(f, phase);
  }

  float output() // 三角波出力 0～1
  {
    float y = 2.0f * saw.output();
    if (y < 1.0f) return y;
    else return 2.0f - y;
  }

};

#endif // LIB_OSC_HPP
