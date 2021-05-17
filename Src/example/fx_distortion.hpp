#ifndef FX_DISTORTION_HPP
#define FX_DISTORTION_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_distortion : public fx_base
{
private:
  const string name = "DISTORTION";
  const uint16_t color = COLOR_R; // 赤
  const string paramName[20] = {"LEVEL", "GAIN", "TONE"};
  enum paramName {LEVEL, GAIN, TONE};
  float param[20] = {1, 1, 1};
  const int16_t paramMax[20] = {100,100,100};
  const int16_t paramMin[20] = {  0,  0,  0};
  const uint8_t paramNumMax = 3;

  signalSw bypass;
  hpf hpf1, hpf2, hpfTone; // ローカット1・2、トーン調整用
  lpf lpf1, lpf2, lpfTone; // ハイカット1・2、トーン調整用

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
        param[LEVEL] = logPot(fxParam[LEVEL], -50.0f, 0.0f);  // LEVEL -50...0 dB
        break;
      case 1:
        param[GAIN] = logPot(fxParam[GAIN], 5.0f, 45.0f); // GAIN 5...+45 dB
        break;
      case 2:
        param[TONE] = mixPot(fxParam[TONE], -20.0f); // TONE 0～1 LPF側とHPF側をミックス
        break;
      case 3:
        lpf1.set(5000.0f); // ハイカット1 固定値
        break;
      case 4:
        lpf2.set(4000.0f); // ハイカット2 固定値
        break;
      case 5:
        lpfTone.set(240.0f); // TONE用ハイカット 固定値
        break;
      case 6:
        hpf1.set(40.0f); // ローカット1 固定値
        break;
      case 7:
        hpf2.set(30.0f); // ローカット2 固定値
        break;
      case 8:
        hpfTone.set(1000.0f); // TONE用ローカット 固定値
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
      fxL[i] = hpf1.process(fxL[i]); // ローカット1
      fxL[i] = lpf1.process(fxL[i]); // ハイカット1
  	  fxL[i] = 10.0f * fxL[i]; // 1段目固定ゲイン

  	  if (fxL[i] < -0.5f) fxL[i] = -0.25f; // 2次関数による波形の非対称変形
  	  else fxL[i] = fxL[i] * fxL[i] + fxL[i];

      fxL[i] = hpf2.process(fxL[i]); // ローカット2 直流カット
      fxL[i] = lpf2.process(fxL[i]); // ハイカット2
  	  fxL[i] = param[GAIN] * fxL[i]; // GAIN

      fxL[i] = tanhf(fxL[i]); // tanhによる対称クリッピング

  	  fxL[i] = param[TONE] * hpfTone.process(fxL[i])        // TONE
  	      + (1.0f - param[TONE]) * lpfTone.process(fxL[i]); // LPF側とHPF側をミックス

      fxL[i] = param[LEVEL] * fxL[i]; // LEVEL

      xL[i] = bypass.process(xL[i], fxL[i], fxOn);
    }
  }

};

#endif // FX_DISTORTION_HPP
