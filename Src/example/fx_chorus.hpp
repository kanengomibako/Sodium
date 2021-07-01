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
  sineWave sin1;
  delayBufF del1;
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

    del1.set(20.0f);  // 最大ディレイタイム設定
    hpf1.set(100.0f); // ディレイ音のローカット設定
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
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f); // LEVEL -20 ～ 20dB
        break;
      case 1:
        param[MIX] = mixPot(fxParam[MIX], -20.0f); // MIX
        break;
      case 2:
        param[FBACK] = (float)fxParam[FBACK] / 100.0f; // Feedback 0～99%
        break;
      case 3:
        param[RATE] = 0.02f * (105.0f - (float)fxParam[RATE]); // RATE 周期 2.1～0.1 秒
        break;
      case 4:
        param[DEPTH] = 0.05f * (float)fxParam[DEPTH]; // Depth ±5ms
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
        sin1.set(1.0f / param[RATE]);
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
      float fxL;
      
      float dtime = 5.0f + param[DEPTH] * (1.0f + sin1.output()); // ディレイタイム 5～15ms
      fxL = del1.readLerp(dtime); // ディレイ音読込(線形補間)
      fxL = lpf2nd1.process(fxL); // ディレイ音のTONE(ハイカット)
      fxL = lpf2nd2.process(fxL);

      // ディレイ音と原音をディレイバッファに書込、原音はローカットして書込
      del1.write(param[FBACK] * fxL + hpf1.process(xL[i]));

      fxL = (1.0f - param[MIX]) * xL[i] + param[MIX] * fxL; // MIX
      fxL = 1.4f * param[LEVEL] * fxL; // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_CHORUS_HPP
