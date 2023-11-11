#ifndef LIB_SAMPLING_HPP
#define LIB_SAMPLING_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

/* アップサンプリング ----------------------------------------------------------*/
class upsampling
{
private:
  uint8_t m;         // m倍アップサンプリング
  uint16_t wpos = 0; // write position アップサンプリング後の配列の書き込み位置
  float x1 = 0.0f;   // 前回のサンプル値
  float fc = 0.0f;   // アップサンプリング時LPFカットオフ周波数
  biquadFilter lpf1, lpf2; // アップサンプリング後の4次Butterworth LPF

public:
  upsampling()
  {
    set(1, 20000.0f);
  }

  upsampling(uint8_t n, float f)
  {
    set(n, f);
  }

  void set(uint8_t n, float f)
  {
    m = n;
    fc = f;
    wpos = 0;
    lpf1.setLPF(fc / (float)m, 0.5411961f); // 4次Butterworth LPF 係数計算
    lpf2.setLPF(fc / (float)m, 1.3065630f);
  }

  void process(float x, float upsampleArray[], uint16_t sampleNum) // アップサンプリング用配列へ変換
  {
    for (uint16_t i = 0; i < m;)
    {
      upsampleArray[wpos] = lerp(x1, x, 1.0f/(float)m * (float)i); // サンプル間を線形補間
      i++;
      if (i == m) x1 = x; // 前回サンプル値x1を入れ替え
      if (fc > 1.0f && m > 1)
      { // アップサンプリング後の配列にLPFをかける
        upsampleArray[wpos] = lpf1.process(upsampleArray[wpos]);
        upsampleArray[wpos] = lpf2.process(upsampleArray[wpos]);
      }
      wpos++;
      if (wpos == sampleNum) wpos = 0; // 最大サンプル数(通常BLOCK_SIZE*m)までで0に戻す
    }
  }

};

/* ダウンサンプリング ----------------------------------------------------------*/
class downsampling
{
private:
  uint8_t m;         // 1/mダウンサンプリング
  uint16_t rpos = 0; // read position アップサンプリング後の配列の読み込み位置
  float fc = 0.0f;   // ダウンサンプリング時LPFカットオフ周波数
  biquadFilter lpf1, lpf2, lpf3, lpf4; // ダウンサンプリング前の8次Butterworth LPF

public:
  downsampling()
  {
    set(1, 20000.0f);
  }

  downsampling(uint8_t n, float f)
  {
    set(n, f);
  }

  void set(uint8_t n, float f)
  {
    m = n;
    fc = f;
    rpos = 0;
    lpf1.setLPF(fc / (float)m, 0.50979558f); // 8次Butterworth LPF 係数計算
    lpf2.setLPF(fc / (float)m, 0.60134489f);
    lpf3.setLPF(fc / (float)m, 0.89997622f);
    lpf4.setLPF(fc / (float)m, 2.56291540f);
  }

  void process(float x, float downsampleArray[], uint16_t sampleNum) // ダウンサンプリング用配列へ変換
  {
    if (fc > 1.0f && m > 1)
    { // ダウンサンプリング前の配列にLPFをかける
      x = lpf1.process(x);
      x = lpf2.process(x);
      x = lpf3.process(x);
      x = lpf4.process(x);
    }
    if (rpos % m == 0) downsampleArray[rpos / m] = x; // 間引きしてダウンサンプリング後の配列へ
    rpos++;
    if (rpos == sampleNum) rpos = 0; // 最大サンプル数(通常BLOCK_SIZE*m)までで0に戻す
  }

};

#endif // LIB_SAMPLING_HPP
