#ifndef FX_PHASER_HPP
#define FX_PHASER_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_osc.hpp"

class fx_phaser : public fx_base
{
private:
  enum paramName {LEVEL, RATE, STAGE,
      P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19};
  float param[20] = {0.0f, 0.0f, 0.0f,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMax[20] = {100,100,6,
      1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
  const int16_t paramMin[20] = {  0, 0, 1,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const string paramName[20] = {
      "LEVEL", "RATE", "STAGE",
      "","","","","","","","","","","","","","","","",""};
  const uint8_t paramIndexMax = 2;

  signalSw bypass;
  triangleWave tri;
  apf apfx[12];

public:
  fx_phaser()
  {
    fxNameList[PH] = "PHASER";
    fxColorList[PH] = 0b1111100000000000; // 赤
  }

  virtual void init()
  {
    fxParamIndexMax = paramIndexMax;
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
      case LEVEL:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case RATE:
        fxParamStr[RATE] = std::to_string(fxParam[RATE]);
        break;
      case STAGE:
        fxParamStr[STAGE] = std::to_string(fxParam[STAGE]*2);
        break;
      default:
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
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f);  // LEVEL -20 ～ 20dB
        break;
      case 1:
        param[RATE] = 0.02f * (105.0f - (float)fxParam[RATE]); // RATE 2s
        break;
      case 2:
        param[STAGE] = 0.1f + (float)fxParam[STAGE] * 2.0f; // STAGE 2, 4, 6, 8, 12
        break;
      case 3:
        tri.set(param[RATE]);
        break;
      default:
        break;
    }
  }

  virtual void process(float xL[], float xR[])
  {
    float fxL[BLOCK_SIZE] = {};

    setParam();

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      fxL[i] = xL[i];
      float freq = 200.0f * dbToGain(20.0f * tri.output()); // APF周波数 200～2000Hz

      for (uint8_t j = 0; j < (uint8_t)param[STAGE]; j++) // 段数分APFをかける
      {
        apfx[j].set(freq); // APF周波数を設定
        fxL[i] = apfx[j].process(fxL[i]); // APF実行
      }
      fxL[i] = 0.7f * (xL[i] + fxL[i]); // 原音ミックス
      xL[i] = bypass.process(xL[i], fxL[i] * param[LEVEL], fxOn);
    }
  }

};

#endif // FX_PHASER_HPP
