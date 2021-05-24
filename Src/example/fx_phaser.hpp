#ifndef FX_PHASER_HPP
#define FX_PHASER_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_osc.hpp"

class fx_phaser : public fx_base
{
private:
  const string name = "PHASER";
  const uint16_t color = COLOR_R; // 赤
  const string paramName[20] = {"LEVEL", "RATE", "STAGE"};
  enum paramName {LEVEL, RATE, STAGE};
  float param[20] = {1, 1, 1};
  const int16_t paramMax[20] = {100,100,  6};
  const int16_t paramMin[20] = {  0,  0,  1};
  const uint8_t paramNumMax = 3;

  signalSw bypass;
  triangleWave tri;
  apf apfx[12];

public:
  virtual void init()
  {
    fxName = name;
    fxColor = color;
    fxParamNumMax = paramNumMax;
    for (int i = 0; i < 20; i++)
    {
      fxParamName[i] = paramName[i];
      fxParamMax[i] = paramMax[i];
      fxParamMin[i] = paramMin[i];
      if (fxAllData[fxNum][i] < paramMin[i] || fxAllData[fxNum][i] > paramMax[i]) fxParam[i] = paramMin[i];
      else fxParam[i] = fxAllData[fxNum][i];
    }
  }

  virtual void deinit()
  {
  }

  virtual void setParamStr(uint8_t paramNum)
  {
    switch(paramNum)
    {
      case 0:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case 1:
        fxParamStr[RATE] = std::to_string(fxParam[RATE]);
        break;
      case 2:
        fxParamStr[STAGE] = std::to_string(fxParam[STAGE] * 2);
        break;
      default:
        fxParamStr[paramNum] = "";
        break;
    }
  }

  virtual void setParam()
  {
    static uint8_t count = 0;
    count = (count + 1) % 10; // 負荷軽減のためパラメータ計算を分散させる
    switch(count)
    {
      case 0:
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f);  // LEVEL -20～20 dB
        break;
      case 1:
        param[RATE] = 0.02f * (105.0f - (float)fxParam[RATE]); // RATE 周期 2～0.1 秒
        break;
      case 2:
        param[STAGE] = 0.1f + (float)fxParam[STAGE] * 2.0f; // STAGE 2～12 後で整数へ変換
        break;
      case 3:
        tri.set(1.0f / param[RATE]); // 三角波 周波数設定
        break;
      default:
        break;
    }
  }

  virtual void process(float xL[], float xR[])
  {
    setParam();

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      float fxL = xL[i];

      float lfo = 20.0f * tri.output();    // LFO 0～20 三角波
      float freq = 200.0f * dbToGain(lfo); // APF周波数 200～2000Hz 指数的変化

      for (uint8_t j = 0; j < (uint8_t)param[STAGE]; j++) // 段数分APF繰り返し
      {
        apfx[j].set(freq);          // APF周波数設定
        fxL = apfx[j].process(fxL); // APF実行
      }

      fxL = 0.7f * (xL[i] + fxL); // 原音ミックス、音量調整
      fxL = param[LEVEL] * fxL;   // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_PHASER_HPP
