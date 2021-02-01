#ifndef FX_TREMOLO_HPP
#define FX_TREMOLO_HPP

#include "common.h"
#include "lib_osc.hpp"
#include "lib_calc.hpp"

class fx_tremolo : public fx_base
{
private:
  enum paramName {LEVEL, RATE, DEPTH, WAVE,
    P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15,P16,P17,P18,P19};
  float param[20] = {0.0f, 1.0f, 1.0f, 1.0f,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMax[20] = {100,100,100,100,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const int16_t paramMin[20] = {  0,  0,  0,  0,
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  const string paramName[20] = {
      "LEVEL", "RATE", "DEPTH",
      "WAVE",
      "","","","","","","","","","","","","","","",""};
  const uint8_t paramIndexMax = 3;

  signalSw bypass;
  triangleWave tri;

public:
  fx_tremolo()
  {
    fxNameList[TR] = "TREMOLO";
    fxColorList[TR] = 0b0000011111111111; // 青緑
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

  virtual void setParamStr(uint8_t paramIndex)
  {
    switch(paramIndex)
    {
      case LEVEL:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case RATE:
        fxParamStr[RATE] = std::to_string(fxParam[RATE]);
        break;
      case DEPTH:
        fxParamStr[DEPTH] = std::to_string(fxParam[DEPTH]);
        break;
      case WAVE:
        fxParamStr[WAVE] = std::to_string(fxParam[WAVE]);
        break;
      default:
        fxParamStr[paramIndex] = "";
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
        param[RATE] = 0.01f * (105.0f - (float)fxParam[RATE]); // Rate 0.05s ～ 1.05s
        break;
      case 2:
        param[DEPTH] = (float)fxParam[DEPTH] * 0.1f; // Depth ±10dB
        break;
      case 3:
        param[WAVE] = logPot(fxParam[WAVE], 0.0f, 50.0f); // Wave 三角波～矩形波変形
        break;
      case 4:
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
      float gain = 2.0f * (tri.output() - 0.5f); // -1 ～ 1 dB LFO
      gain = clip(gain * param[WAVE], -1.0f, 1.0f); // 三角波～矩形波変形
      gain = gain * param[DEPTH]; // gain -10 ～ 10 dB

      fxL[i] = param[LEVEL] * xL[i] * dbToGain(gain); // LEVEL
      xL[i] = bypass.process(xL[i], fxL[i], sw[4]);
    }
  }

};

#endif // FX_TREMOLO_HPP
