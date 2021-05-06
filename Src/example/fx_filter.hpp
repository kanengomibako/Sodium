#ifndef FX_FILTER_HPP
#define FX_FILTER_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_filter : public fx_base
{
private:
  const string name = "FILTER";
  const uint16_t color = COLOR_G; // 緑
  const string paramName[20] = {"LV", "LPF", "HPF"};
  enum paramName {LEVEL, LPF, HPF};
  float param[20] = {1, 1, 1};
  const int16_t paramMax[20] = { 20, 99,100};
  const int16_t paramMin[20] = {-20,  1,  1};
  const uint8_t paramNumMax = 3;

  signalSw bypass;
  lpf lpf1;
  hpf hpf1;

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
        if (fxParam[LEVEL] > 0) fxParamStr[LEVEL] = "+" + fxParamStr[LEVEL];
        break;
      case 1:
        fxParamStr[LPF] = std::to_string(fxParam[LPF] * 100);
        break;
      case 2:
        fxParamStr[HPF] = std::to_string(fxParam[HPF] * 10);
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
        param[LEVEL] = dbToGain(fxParam[LEVEL]); // LEVEL -20...+20 dB
        break;
      case 1:
        param[LPF] = (float)fxParam[LPF] * 100.0f; // 1次LPF カットオフ周波数 100...9900 Hz
        lpf1.set(param[LPF]);
        break;
      case 2:
        param[HPF] = (float)fxParam[HPF] * 10.0f; // 1次HPF カットオフ周波数 10...1000 Hz
        hpf1.set(param[HPF]);
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
      fxL[i] = lpf1.process(fxL[i]); // 1次LPF
      fxL[i] = hpf1.process(fxL[i]); // 1次HPF
  	  fxL[i] = param[LEVEL] * fxL[i]; // LEVEL
      xL[i] = bypass.process(xL[i], fxL[i], fxOn);
    }
  }

};

#endif // FX_FILTER_HPP
