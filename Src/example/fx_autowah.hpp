#ifndef FX_AUTOWAH_HPP
#define FX_AUTOWAH_HPP

#include "common.h"
#include "lib_calc.hpp"
#include "lib_filter.hpp"

class fx_autowah : public fx_base
{
private:
  const string name = "AUTOWAH";
  const uint16_t color = COLOR_G; // 緑
  const string paramName[20] = {"LEVEL", "SENS", "Q", "HiF", "LowF"};
  enum paramName {LEVEL, SENS, Q, HIFREQ, LOFREQ};
  float param[20] = {1, 1, 1, 1, 1};
  const int16_t paramMax[20] = {100,100, 90, 90, 90};
  const int16_t paramMin[20] = {  0,  0, 10, 10, 10};
  const uint8_t paramNumMax = 5;

  signalSw bypass;
  lpf lpf1, lpf2; // エンベロープ用ローパスフィルタ
  biquadFilter bpf1; // ワウ用バンドパスフィルタ
  float logFreqRatio = 0.6f; // バンドパスフィルタ中心周波数計算用変数

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

    lpf1.set(20.0f); // エンベロープ用ローパスフィルタ設定
    lpf2.set(50.0f);
    bpf1.setBPF(1000.0f, 1.0f); // ワウ用バンドパスフィルタ初期値
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
        fxParamStr[SENS] = std::to_string(fxParam[SENS]);
        break;
      case 2:
        fxParamStr[Q] = std::to_string(fxParam[Q]);
        fxParamStr[Q].insert(1, ".");
        break;
      case 3:
        fxParamStr[HIFREQ] = std::to_string(fxParam[HIFREQ] * 100);
        break;
      case 4:
        fxParamStr[LOFREQ] = std::to_string(fxParam[LOFREQ] * 10);
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
        param[LEVEL] = logPot(fxParam[LEVEL], 0.0f, 30.0f); // LEVEL 0～+30dB
        break;
      case 1:
        param[SENS] = 0.5f * fxParam[SENS]; // SENSITIVITY 0～+50dB
        break;
      case 2:
        param[Q] = (float)fxParam[Q] / 10.0f; // Q 1.0～9.0
        break;
      case 3:
        param[HIFREQ] = (float)fxParam[HIFREQ] * 100.0f; // HIGH FREQ BPF中心周波数 最高 1000～9000Hz
        break;
      case 4:
        param[LOFREQ] = (float)fxParam[LOFREQ] * 10.0f; // LOW FREQ BPF中心周波数 最低 100～900Hz
        break;
      case 5:
        logFreqRatio = log10f(param[HIFREQ] / param[LOFREQ]); // BPF周波数変化比の常用対数
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

      float env = abs(fxL);
      env = lpf1.process(env); // 絶対値とLPFでエンベロープ取得
      env = gainToDb(env);     // dB換算
      env = env + param[SENS]; // SENSITIVITY 感度(エンベロープ補正)
      if (env > 0.0f) env = 0.0f; // -20～0dBまででクリップ
      if (env < -20.0f) env = -20.0f;

      env = lpf2.process(env); // 急激な変化を避ける

      float freq = param[HIFREQ] * dbToGain(logFreqRatio * env); // エンベロープに応じた周波数を計算 指数的変化
      bpf1.setBPF(freq, param[Q]); // フィルタ周波数を設定

      fxL = bpf1.process(fxL);  // フィルタ(ワウ)実行
      fxL = param[LEVEL] * fxL; // LEVEL

      xL[i] = bypass.process(xL[i], fxL, fxOn);
    }
  }

};

#endif // FX_AUTOWAH_HPP
