#ifndef FX_CHORUS_HPP
#define FX_CHORUS_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"
#include "lib_osc.hpp"

class fx_chorus : public fx_base
{
private:
  const string name = "CHORUS";
  const uint16_t color = COLOR_B; // 青
  const string paramName[20] = {"LEVEL", "MIX", "F.BACK", "RATE", "DEPTH", "TONE"};
  enum paramName {LEVEL, MIX, FBACK, RATE, DEPTH, TONE};
  float param[20] = {1, 1, 1, 1, 1, 1};
  const int16_t paramMax[20] = {100,100, 99,100,100,100};
  const int16_t paramMin[20] = {  0,  0,  0,  0,  0,  0};
  const uint8_t paramNumMax = 6;

  signalSw bypass;
  triangleWave tri1;
  delayBuf del1;
  hpf hpf1;
  lpf2nd lpf2nd1, lpf2nd2;

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

    del1.set(20.0f); // 最大ディレイタイム設定
    hpf1.set(100.0f); // ウェット音のローカット設定
  }

  virtual void deinit()
  {
    del1.erase();
  }

  virtual void setParamStr(uint8_t paramNum)
  {
    switch(paramNum)
    {
      case 0:
        fxParamStr[LEVEL] = std::to_string(fxParam[LEVEL]);
        break;
      case 1:
        fxParamStr[MIX] = std::to_string(fxParam[MIX]);
        break;
      case 2:
        fxParamStr[FBACK] = std::to_string(fxParam[FBACK]);
        break;
      case 3:
        fxParamStr[RATE] = std::to_string(fxParam[RATE]);
        break;
      case 4:
        fxParamStr[DEPTH] = std::to_string(fxParam[DEPTH]);
        break;
      case 5:
        fxParamStr[TONE] = std::to_string(fxParam[TONE]);
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
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f);  // LEVEL -20 ～ 20dB
        break;
      case 1:
        param[MIX] = mixPot(fxParam[MIX], -20.0f);  // MIX
        break;
      case 2:
        param[FBACK] = (float)fxParam[FBACK] / 100.0f; // Feedback 0～0.99
        break;
      case 3:
        param[RATE] = 0.02f * (105.0f - (float)fxParam[RATE]); // Rate 2s
        break;
      case 4:
        param[DEPTH] = 0.1f * (float)fxParam[DEPTH]; // Depth 10ms
        break;
      case 5:
        param[TONE] = 800.0f * logPot(fxParam[TONE], 0.0f, 20.0f); // HI CUT FREQ 800 ～ 8000 Hz
        break;
      case 6:
        lpf2nd1.set(param[TONE]);
        break;
      case 7:
        lpf2nd2.set(param[TONE]);
        break;
      case 8:
        tri1.set(1.0f / param[RATE]);
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

      float dtime = param[DEPTH] * tri1.output() + 5.0f; // ディレイタイム5~15ms
      fxL = del1.readLerp(dtime);
      fxL = lpf2nd1.process(fxL);
      fxL = lpf2nd2.process(fxL);
      del1.write(hpf1.process(xL[i]) + param[FBACK] * fxL);

      fxL = (1.0f - param[MIX]) * xL[i] + param[MIX] * fxL; // 原音ミックス
      fxL = param[LEVEL] * 1.4f * fxL; // LEVEL、音量調整
      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_CHORUS_HPP
