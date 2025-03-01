#ifndef FX_UPSAMPLING_HPP
#define FX_UPSAMPLING_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"
#include "lib_sampling.hpp"

class fx_upsampling : public fx_base
{
private:
  const string name = "UPSAMPLING";
  const uint16_t color = COLOR_RG; // 赤緑
  const string paramName[20] = {"LEVEL", "GAIN", "UP", "UP_LPF", "DN_LPF"};
  enum paramName {LEVEL, GAIN, UP, UP_LPF, DN_LPF};
  float param[20] = {1, 1, 1, 1, 1};
  const int16_t paramMax[20] = {100,100,  8, 22, 40};
  const int16_t paramMin[20] = {  0,  0,  1,  0,  0};
  const uint8_t paramNumMax = 5;

  signalSw bypass;
  upsampling up;
  downsampling down;
  float upArray[8*BLOCK_SIZE]; // 8倍分のアップサンプリング用配列
  float downArray[1*BLOCK_SIZE]; // ダウンサンプリング用配列

public:
  void init() override
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

  void deinit() override
  {
  }

  void setParamStr(uint8_t paramNum) override
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
        fxParamStr[UP] = std::to_string(fxParam[UP]);
        break;
      case 3:
        fxParamStr[UP_LPF] = std::to_string(fxParam[UP_LPF]);
        break;
      case 4:
        fxParamStr[DN_LPF] = std::to_string(fxParam[DN_LPF]);
        break;
      default:
        fxParamStr[paramNum] = "";
        break;
    }
  }

  void setParam() override
  {
    static uint8_t count = 0;
    count = (count + 1) % 10; // 負荷軽減のためパラメータ計算を分散させる
    switch(count)
    {
      case 0:
        param[LEVEL] = logPot(fxParam[LEVEL], -20.0f, 20.0f);  // LEVEL -20～20 dB
        break;
      case 1:
        param[GAIN] = logPot(fxParam[GAIN], 0.0f, 20.0f); // GAIN 0～20 dB
        break;
      case 2:
        param[UP] = (float)fxParam[UP]; // アップサンプリング倍率 1～8倍
        break;
      case 3:
        param[UP_LPF] = 1000.0f * (float)fxParam[UP_LPF]; // アップサンプリング時LPFカットオフ周波数 1k～22kHz（0でオフ）
        break;
      case 4:
        param[DN_LPF] = 1000.0f * (float)fxParam[DN_LPF]; // ダウンサンプリング時LPFカットオフ周波数 1k～40kHz（0でオフ）
        break;
      default:
        break;
    }
  }

  void process(float xL[], float xR[]) override
  {
    setParam();
    up.set(fxParam[UP], param[UP_LPF]);
    down.set(fxParam[UP], param[DN_LPF]);

    float fxL;

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      up.process(xL[i], upArray, fxParam[UP] * BLOCK_SIZE); // アップサンプリングした配列作成
    }

    for (uint16_t i = 0; i < fxParam[UP] * BLOCK_SIZE; i++)
    { // アップサンプリングした配列を処理
      float x = param[GAIN] * upArray[i]; // GAIN
      if (x < -0.5f) x = -0.5f; // -0.5～0.5でクリッピング
      if (x >  0.5f) x =  0.5f;
      down.process(x, downArray, fxParam[UP] * BLOCK_SIZE); // ダウンサンプリングした配列作成
    }

    for (uint16_t i = 0; i < BLOCK_SIZE; i++)
    {
      fxL = downArray[i];
      fxL = param[LEVEL] * fxL; // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_UPSAMPLING_HPP
