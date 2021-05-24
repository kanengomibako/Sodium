#ifndef FX_OSCILLATOR_HPP
#define FX_OSCILLATOR_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_osc.hpp"

class fx_oscillator : public fx_base
{
private:
  const string name = "OSCILLATOR";
  const uint16_t color = COLOR_B; // 青
  const string paramName[20] = {"LEVEL", "FREQ", "TYPE"};
  enum paramName {LEVEL, FREQ, TYPE};
  float param[20] = {1, 1, 1};
  const int16_t paramMax[20] = {100,200,  2};
  const int16_t paramMin[20] = {  0,  2,  0};
  const uint8_t paramNumMax = 3;

  const string typeName[3] = {"SAW","TRI","SIN"};
  signalSw bypass;
  sawWave saw;
  sineWave sin;
  triangleWave tri;

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
        fxParamStr[FREQ] = std::to_string((uint16_t)param[FREQ]);
        break;
      case 2:
        fxParamStr[TYPE] = typeName[fxParam[TYPE]];
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
        param[LEVEL] = logPot(fxParam[LEVEL], -50.0f, 0.0f);  // LEVEL -50～0 dB
        break;
      case 1:
        param[FREQ] = 10.0f * (float)fxParam[FREQ]; // 周波数 20～2000 Hz
        saw.set(param[FREQ]);
        tri.set(param[FREQ]);
        sin.set(param[FREQ]);
        break;
      case 2:
        param[TYPE] = (float)fxParam[TYPE]; // TYPE
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

      if (fxParam[TYPE] == 0) fxL = 2.0f * saw.output() - 1.0f;
      if (fxParam[TYPE] == 1) fxL = 2.0f * tri.output() - 1.0f;
      if (fxParam[TYPE] == 2) fxL = sin.output();

      fxL = param[LEVEL] * fxL; // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_OSCILLATOR_HPP
