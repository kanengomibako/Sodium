#ifndef LIB_DELAY_HPP
#define LIB_DELAY_HPP

#include "common.h"

/* ディレイバッファ ----------------------------------------*/
class delayBuf
{
private:
  int16_t *delayArray;
  uint32_t wpos = 0; // write position 書き込み位置
  uint32_t maxDelaySample = 1;

public:
  delayBuf()
  {
    set(1.0f);
  }

  ~delayBuf()
  {
    erase();
  }

  void set(float maxDelayTime) // 最大ディレイ時間 ms メモリ確保
  {
    // メモリ確保失敗検知 未実装
    // std::bad_alloc を使う場合は -fexceptions コンパイルオプションが必要

    erase();
    maxDelaySample = (uint32_t)(SAMPLING_FREQ * maxDelayTime / 1000.0f); // 最大サンプル数計算
    delayArray = new int16_t[maxDelaySample]; // バッファ配列メモリ確保
    for (uint32_t i = 0; i < maxDelaySample; i++) delayArray[i] = 0; // 配列0埋め
  }

  void erase()
  {
    delete[] delayArray; // バッファ配列メモリ削除
  }

  void write(float x) // バッファ書き込み
  {
    if (x < -1.0f) x = -1.0f; // オーバーフロー防止
    if (x > 0.999f) x = 0.999f;
    delayArray[wpos] = (int16_t)(x * 32767.0f); // バッファ配列へ16ビット整数で書き込み
    wpos++; // 書込位置を1つ進める
    if (wpos == maxDelaySample) wpos = 0; // 最大サンプル数までで0に戻す
  }

  float read(float delayTime) // 通常のサンプル単位での読み出し
  {
    uint32_t interval = (uint32_t) (0.001f * delayTime * SAMPLING_FREQ); // 書込位置と読出位置の間隔を計算
    if (interval > maxDelaySample) interval = maxDelaySample;
    uint32_t rpos; // read position 読出位置
    if (wpos >= interval) rpos = wpos - interval;  // 読出位置を取得
    else rpos = wpos - interval + maxDelaySample;
    return ((float) delayArray[rpos]) / 32767.0f; // バッファ配列からfloatで読み出し
  }

  float readLerp(float delayTime) // 線形補間して読み出し コーラス等に利用
  {
    float intervalF = 0.001f * delayTime * SAMPLING_FREQ; // 書込位置と読出位置の間隔をfloatで計算
    if (intervalF > (float)maxDelaySample) intervalF = (float)maxDelaySample;
    float rposF; // read position 読出位置 float
    if ((float)wpos >= intervalF) rposF = (float)wpos - intervalF; // 読出位置をfloatで取得
    else rposF = (float)wpos - intervalF + (float)maxDelaySample;
    uint32_t rpos0 = (uint32_t)rposF; // 読出位置の前後の整数値を取得
    uint32_t rpos1 = rpos0 + 1;
    if (rpos1 == maxDelaySample) rpos1 = 0;
    float t = rposF - (float)rpos0;  // 線形補間用係数
    return ((float)delayArray[rpos0] + t * (float)(delayArray[rpos1] - delayArray[rpos0])) / 32767.0f;
  }

  float readFixed() // 固定時間（最大ディレイタイム）で読み出し
  {
    return ((float) delayArray[wpos]) / 32767.0f; // バッファ配列からfloatで読み出し
  }

};

#endif // LIB_DELAY_HPP
