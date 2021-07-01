#ifndef LIB_DELAY_HPP
#define LIB_DELAY_HPP

#include "common.h"

/* ディレイバッファ 16ビット整数 -------------------------------------*/
class delayBuf
{
private:
  int16_t *delayArray; // ディレイバッファ配列
  uint32_t wpos = 0;   // write position 書き込み位置
  uint32_t maxDelaySample = 1; // 最大ディレイタイム サンプル数

public:
  void set(float maxDelayTime) // バッファ配列メモリ確保 最大ディレイ時間 ms
  { // ※メモリ確保失敗検知未実装 std::bad_alloc を使う場合は -fexceptions コンパイルオプションが必要
    maxDelaySample = 1 + (uint32_t)(SAMPLING_FREQ * maxDelayTime / 1000.0f); // 最大サンプル数計算
    delayArray = new int16_t[maxDelaySample](); // メモリ確保（0初期化）
  }

  void erase() // バッファ配列メモリ削除
  {
    delete[] delayArray;
  }

  void write(float x) // バッファ配列書き込み、書込位置を進める
  {
    if (x < -1.00f) x = -1.00f; // オーバーフロー防止
    if (x > 0.999f) x = 0.999f;
    delayArray[wpos] = (int16_t)(x * 32768.0f); // 16ビット整数で書き込み
    wpos++; // 書込位置を1つ進める
    if (wpos == maxDelaySample) wpos = 0; // 最大サンプル数までで0に戻す
  }

  float read(float delayTime) // 通常のサンプル単位での読み出し ディレイ時間 ms
  {
    uint32_t interval = (uint32_t)(0.001f * delayTime * SAMPLING_FREQ); // 書込位置と読出位置の間隔を計算
    if (interval > maxDelaySample) interval = maxDelaySample; // 最大ディレイ時間を超えないようにする
    uint32_t rpos; // read position 読出位置
    if (wpos >= interval) rpos = wpos - interval; // 読出位置を取得
    else rpos = wpos + maxDelaySample - interval;
    return (float)delayArray[rpos] / 32768.0f; // バッファ配列からfloat(-1～1)で読み出し
  }

  float readLerp(float delayTime) // 線形補間して読み出し コーラス等に利用
  {
    float intervalF = 0.001f * delayTime * SAMPLING_FREQ; // 書込位置と読出位置の間隔を計算 float
    if (intervalF > (float)maxDelaySample) intervalF = (float)maxDelaySample; // 最大ディレイ時間を超えないようにする
    float rposF; // read position 読出位置 float
    if ((float)wpos >= intervalF) rposF = (float)wpos - intervalF; // 読出位置を取得 float
    else rposF = (float)(wpos + maxDelaySample) - intervalF;
    uint32_t rpos0 = (uint32_t)rposF; // 読出位置の前後の整数値を取得
    uint32_t rpos1 = rpos0 + 1;
    if (rpos1 == maxDelaySample) rpos1 = 0; // 読み出し位置最大と0にまたがる場合
    float y0 = (float)delayArray[rpos0]; // 読出位置の前後のデータ 線形補間用一時変数
    float y1 = (float)delayArray[rpos1];
    return (y0 + (rposF - (float)rpos0) * (y1 - y0)) / 32768.0f; // 線形補間値をfloat(-1～1)で読み出し
  }

  float readFixed() // 固定時間（最大ディレイタイム）で読み出し
  {
    return (float)delayArray[wpos] / 32768.0f; // バッファ配列からfloat(-1～1)で読み出し
  }

};

/* ディレイバッファ 32ビット float -------------------------------------*/
class delayBufF
{
private:
  float *delayArray; // ディレイバッファ配列 float
  uint32_t wpos = 0;   // write position 書き込み位置
  uint32_t maxDelaySample = 1; // 最大ディレイタイム サンプル数

public:
  void set(float maxDelayTime) // バッファ配列メモリ確保 最大ディレイ時間 ms
  { // ※メモリ確保失敗検知未実装 std::bad_alloc を使う場合は -fexceptions コンパイルオプションが必要
    maxDelaySample = 1 + (uint32_t)(SAMPLING_FREQ * maxDelayTime / 1000.0f); // 最大サンプル数計算
    delayArray = new float[maxDelaySample](); // メモリ確保（0初期化）
  }

  void erase() // バッファ配列メモリ削除
  {
    delete[] delayArray;
  }

  void write(float x) // バッファ配列書き込み、書込位置を進める
  {
    delayArray[wpos] = x; // floatのまま書き込み
    wpos++; // 書込位置を1つ進める
    if (wpos == maxDelaySample) wpos = 0; // 最大サンプル数までで0に戻す
  }

  float read(float delayTime) // 通常のサンプル単位での読み出し ディレイ時間 ms
  {
    uint32_t interval = (uint32_t)(0.001f * delayTime * SAMPLING_FREQ); // 書込位置と読出位置の間隔を計算
    if (interval > maxDelaySample) interval = maxDelaySample; // 最大ディレイ時間を超えないようにする
    uint32_t rpos; // read position 読出位置
    if (wpos >= interval) rpos = wpos - interval; // 読出位置を取得
    else rpos = wpos + maxDelaySample - interval;
    return delayArray[rpos]; // バッファ配列読み出し
  }

  float readLerp(float delayTime) // 線形補間して読み出し コーラス等に利用
  {
    float intervalF = 0.001f * delayTime * SAMPLING_FREQ; // 書込位置と読出位置の間隔を計算 float
    if (intervalF > (float)maxDelaySample) intervalF = (float)maxDelaySample; // 最大ディレイ時間を超えないようにする
    float rposF; // read position 読出位置 float
    if ((float)wpos >= intervalF) rposF = (float)wpos - intervalF; // 読出位置を取得 float
    else rposF = (float)(wpos + maxDelaySample) - intervalF;
    uint32_t rpos0 = (uint32_t)rposF; // 読出位置の前後の整数値を取得
    uint32_t rpos1 = rpos0 + 1;
    if (rpos1 == maxDelaySample) rpos1 = 0; // 読み出し位置最大と0にまたがる場合
    float t = rposF - (float)rpos0; // 線形補間用係数
    return (1 - t) * delayArray[rpos0] + t * delayArray[rpos1]; // 線形補間値を読み出し
  }

  float readFixed() // 固定時間（最大ディレイタイム）で読み出し
  {
    return delayArray[wpos]; // バッファ配列から読み出し
  }

};

#endif // LIB_DELAY_HPP
