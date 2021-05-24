#ifndef FX_TREMOLO_HPP
#define FX_TREMOLO_HPP

#include "common.h"
#include "lib_osc.hpp"
#include "lib_calc.hpp"

class fx_tremolo : public fx_base
{
private:
  const string name = "TREMOLO";
  const uint16_t color = COLOR_BG; // 青緑
  const string paramName[20] = {"LEVEL", "RATE", "DEPTH", "WAVE"};
  enum paramName {LEVEL, RATE, DEPTH, WAVE};
  float param[20] = {1, 1, 1, 1};
  const int16_t paramMax[20] = {100,100,100,100};
  const int16_t paramMin[20] = {  0,  0,  0,  0};
  const uint8_t paramNumMax = 4;

  signalSw bypass;
  triangleWave tri;

public:
  virtual void init()
  {
    fxParamNumMax = paramNumMax;
    for (int i = 0; i < 20; i++)
    {
      fxName = name;
      fxColor = color;
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
        fxParamStr[DEPTH] = std::to_string(fxParam[DEPTH]);
        break;
      case 3:
        fxParamStr[WAVE] = std::to_string(fxParam[WAVE]);
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
        param[RATE] = 0.01f * (105.0f - (float)fxParam[RATE]); // RATE 周期 1.05～0.05 秒
        break;
      case 2:
        param[DEPTH] = (float)fxParam[DEPTH] * 0.1f; // DEPTH -10～10 dB
        break;
      case 3:
        param[WAVE] = logPot(fxParam[WAVE], 0.0f, 50.0f); // WAVE 三角波～矩形波変形
        break;
      case 4:
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

      float gain = 2.0f * tri.output() - 1.0f; // LFO -1～1 三角波
      gain = param[WAVE] * gain;     // 三角波を増幅
      if (gain > 1.0f) gain = 1.0f;  // クリッピング（矩形波に近い形へ）
      if (gain < -1.0f) gain = -1.0f;
      gain = param[DEPTH] * gain; // DEPTH -10～10 dB

      fxL = dbToGain(gain) * fxL; // 音量を揺らす
      fxL = param[LEVEL] * fxL;   // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_TREMOLO_HPP
