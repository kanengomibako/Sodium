#ifndef FX_OVERDRIVE_HPP
#define FX_OVERDRIVE_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_overdrive : public fx_base
{
private:
  const string name = "OVERDRIVE";
  const uint16_t color = COLOR_RG; // 赤緑
  const string paramName[20] = {"LEVEL", "GAIN", "TREBLE", "BASS"};
  enum paramName {LEVEL, GAIN, TREBLE, BASS};
  float param[20] = {1, 1, 1, 1};
  const int16_t paramMax[20] = {100,100,100,100};
  const int16_t paramMin[20] = {  0,  0,  0,  0};
  const uint8_t paramNumMax = 4;

  signalSw bypass;
  hpf hpfFixed, hpfBass; // 出力ローカット、入力BASS調整
  lpf lpfFixed, lpfTreble; // 入力ハイカット、出力TREBLE調整

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
        fxParamStr[GAIN] = std::to_string(fxParam[GAIN]);
        break;
      case 2:
        fxParamStr[TREBLE] = std::to_string(fxParam[TREBLE]);
        break;
      case 3:
        fxParamStr[BASS] = std::to_string(fxParam[BASS]);
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
        param[GAIN] = logPot(fxParam[GAIN], 20.0f, 60.0f); // GAIN 20～60 dB
        break;
      case 2:
        param[TREBLE] = 10000.0f * logPot(fxParam[TREBLE], -30.0f, 0.0f); // TREBLE LPF 320～10k Hz
        break;
      case 3:
        param[BASS] = 2000.0f * logPot(fxParam[BASS], 0.0f, -20.0f); // BASS HPF 200～2000 Hz
        break;
      case 4:
        lpfTreble.set(param[TREBLE]);
        break;
      case 5:
        hpfBass.set(param[BASS]);
        break;
      case 6:
        lpfFixed.set(4000.0f); // 入力ハイカット 固定値
        break;
      case 7:
        hpfFixed.set(30.0f); // 出力ローカット 固定値
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
      fxL[i] = hpfBass.process(fxL[i]);   // 入力ローカット BASS
      fxL[i] = lpfFixed.process(fxL[i]);  // 入力ハイカット 固定値
  	  fxL[i] = param[GAIN] * fxL[i];      // GAIN
      fxL[i] = atanf(fxL[i] + 0.5f);      // arctanによるクリッピング、非対称化
      fxL[i] = hpfFixed.process(fxL[i]);  // 出力ローカット 固定値 直流カット
      fxL[i] = lpfTreble.process(fxL[i]); // 出力ハイカット TREBLE
  	  fxL[i] = param[LEVEL] * fxL[i];     // LEVEL

      xL[i] = bypass.process(xL[i], fxL[i], fxOn);
    }
  }

};

#endif // FX_OVERDRIVE_HPP
