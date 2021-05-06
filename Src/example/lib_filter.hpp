#ifndef LIB_FILTER_HPP
#define LIB_FILTER_HPP

#include "common.h"
#include "lib_calc.hpp"

/* 2 * PI * fc / fs 計算 -----------------------------*/
inline float omega(float fc)
{
  return 2.0f * PI * fc / SAMPLING_FREQ;
}

/* 1次LPF、HPF、APF用の係数計算、50～10kHzでの近似曲線 -----------------------------*/
inline float lpfCoef(float fc)
{
  float w = omega(fc);
  return 0.99983075f - 0.99678388f * w + 0.49528899f *w*w - 0.10168296f *w*w*w;
}

/* 1次 Low Pass Filter ----------------------------------------------------------*/
class lpf
{
private:
  float b0, a1, y1 = 0;

public:
  lpf()
  {
    set(20000.0f);
  }

  lpf(float fc)
  {
    set(fc);
  }

  void set(float fc)
  {
    a1 = lpfCoef(fc);
    b0 = 1.0f - a1;
  }

  float process(float x)
  {
    float y = b0 * x + a1 * y1;
    y1 = y;
    return y;
  }

};

/* 1次 High Pass Filter ----------------------------------------------------------*/
class hpf
{
private:
  float b0, a1, x1 = 0, y1 = 0;

public:
  hpf()
  {
    set(1.0f);
  }

  hpf(float fc)
  {
    set(fc);
  }

  void set(float fc)
  {
    a1 = lpfCoef(fc);
    b0 = 0.5f * (1.0f + a1);
  }

  float process(float x)
  {
    float y = b0 * x - b0 * x1 + a1 * y1;
    x1 = x;
    y1 = y;
    return y;
  }

};

/* 1次 All Pass Filter ----------------------------------------------------------*/
class apf
{
private:
  float a, x1 = 0, y1 = 0;

public:
  apf()
  {
    set(1.0f);
  }

  apf(float fc)
  {
    set(fc);
  }

  void set(float fc)
  {
    a = lpfCoef(fc);
  }

  float process(float x)
  {
    float y = -a * x + x1 + a * y1;
    x1 = x;
    y1 = y;
    return y;
  }

};

/* 2次 Low Pass Filter ----------------------------------------------------------*/
class lpf2nd
{
private:
  float b, a, y1 = 0, y2 = 0;

public:
  lpf2nd()
  {
    set(20000.0f);
  }

  lpf2nd(float fc)
  {
    set(fc);
  }

  void set(float fc)
  {
    a = lpfCoef(fc);
    b = 1.0f - a;
  }

  float process(float x)
  {
    float y = b * b * x + 2.0f * a * y1 - a * a * y2;
    y2 = y1;
    y1 = y;
    return y;
  }

};

/* 2次 High Pass Filter ----------------------------------------------------------*/
class hpf2nd
{
private:
  float c, a, x1 = 0, x2 = 0, y1 = 0, y2 = 0;

public:
  hpf2nd()
  {
    set(1.0f);
  }

  hpf2nd(float fc)
  {
    set(fc);
  }

  void set(float fc)
  {
    a = lpfCoef(fc);
    c = 0.5f * (1.0f + a);
  }

  float process(float x)
  {
    float y = c * c * (x - 2.0f * x1 + x2) + 2.0 * a * y1 - a * a * y2;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
  }

};

/* BiQuadフィルタ ----------------------------------------------------------*/

// BiQuadフィルタ係数 20～20kHzでの近似計算
inline float bqCosOmega(float fc); // プロトタイプ宣言

inline float bqSinOmega(float fc)
{
  float w;
  if (fc < SAMPLING_FREQ / 10.0f)
  {
    w = omega(fc);
    return - 0.0000070241321f + 1.00045830f * w
           - 0.0038841709f *w*w - 0.15837971f *w*w*w;
  }
  else if (fc < SAMPLING_FREQ / 4.0f)
  {
    w = omega(fc);
    return - 0.032685534f + 1.13017828f * w
           - 0.17781076f *w*w - 0.078404483f *w*w*w;
  }
  else return bqCosOmega(fc - SAMPLING_FREQ / 4.0f);
}

inline float bqCosOmega(float fc)
{
  float w;
  if (fc < SAMPLING_FREQ / 20.0f)
  {
    w = omega(fc);
    return 1.0f - 0.5f *w*w;
  }
  else if (fc < SAMPLING_FREQ / 4.0f)
  {
    w = omega(fc);
    return 0.987157f + 0.075570696f* w
           - 0.65302817f *w*w + 0.13027421f *w*w*w;
  }
  else return - bqSinOmega(fc - SAMPLING_FREQ / 4.0f);
}

inline float bqAlphaQ(float fc,float q)
{
  return bqSinOmega(fc) / ( 2.0f * q );
}

inline float bqAlphaBW(float fc,float bw) // 不使用
{ // 100～10kHz Fs = 48kHz での近似計算
  return ( 4.66355e-6f * bw * bw + 4.15256e-5f * bw + 7.04497e-7f) * fc;
}

inline float bqA(float gain)
{
  return sqrtf(dbToGain(gain));
}

inline float bqBeta(float gain,float q)
{
  return sqrtf(bqA(gain)) / q;
}

enum BQFtype
{
  OFF, // No Filter (default)
  PF,  // Peaking Filter
  LSF, // Low Shelf Filter
  HSF, // High Shelf Filter
  LPF, // Low Pass Filter
  HPF, // High Pass Filter
  BPF, // Band Pass Filter
  NF,  // Notch Filter
  APF, // All Pass Filter
};

class biquadFilter
{
private:
  float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
  float a1, a2, b0, b1, b2;

public:

  biquadFilter() // コンストラクタ
  {
    setCoef(0, 100, 1, 0.0f);
  }

  biquadFilter(int type, float fc, float q_bw) // コンストラクタ 引数にgainなし
  {
    setCoef(type, fc, q_bw, 0.0f);
  }

  biquadFilter(int type, float fc, float q_bw, float gain) // コンストラクタ 引数にgainあり
  {
    setCoef(type, fc, q_bw, gain);
  }

  float process(float x) // BiQuadフィルタ 実行
  {
    float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
  }

  void setCoef(int type, float fc, float q_bw, float gain) // 係数設定
  {
    switch(type)
    {
    case PF:
      setPF(fc, q_bw, gain);
      break;
    case LPF:
   	  setLPF(fc, q_bw);
      break;
    case HPF:
  	  setHPF(fc, q_bw);
      break;
    case BPF:
   	  setBPF(fc, q_bw);
      break;
    case LSF:
      setLSF(fc, q_bw, gain);
      break;
    case HSF:
      setHSF(fc, q_bw, gain);
      break;
    case NF:
   	  setNF(fc, q_bw);
      break;
    case APF:
  	  setAPF(fc, q_bw);
      break;
    default:
      a1 = 0.0f;
      a2 = 0.0f;
      b0 = 1.0f;
      b1 = 0.0f;
      b2 = 0.0f;
    break;
    }
  }

  // 各フィルタタイプ 係数設定
  void setPF(float fc, float q, float gain)
  {
    float alpha = bqAlphaQ(fc,q);
    float cos = bqCosOmega(fc);
    float A = bqA(gain);

    float norm = 1.0f / (1.0f + alpha / A);
    a1 = (-2.0f * cos) * norm;
    a2 = ( 1.0f - alpha / A) * norm;
    b0 = ( 1.0f + alpha * A) * norm;
    b1 = a1;
    b2 = ( 1.0f - alpha * A) * norm;
  }

  void setLPF(float fc, float q)
  {
    float alpha = bqAlphaQ(fc,q);
    float cos = bqCosOmega(fc);

    float norm = 1.0f / (1.0f + alpha);
    a1 = (-2.0f * cos) * norm;
    a2 = ( 1.0f - alpha) * norm;
    b0 = ((1.0f - cos) / 2.0f) * norm;
    b1 = 2.0f * b0 * norm;
    b2 = b0;
  }

  void setHPF(float fc, float q)
  {
    float alpha = bqAlphaQ(fc,q);
    float cos = bqCosOmega(fc);

    float norm = 1.0f / (1.0f + alpha);
    a1 = ( -2.0f * cos) * norm;
    a2 = (  1.0f - alpha) * norm;
    b0 = ( (1.0f + cos) / 2.0f) * norm;
    b1 =  -(1.0f + cos) * norm;
    b2 = b0;
  }

  void setBPF(float fc, float q)
  {
    //float alpha = bqAlphaBW(fc,bw);
    float alpha = bqAlphaQ(fc,q);
    float cos = bqCosOmega(fc);

    float norm = 1.0f / (1.0f + alpha);
    a1 = (-2.0f * cos) * norm;
    a2 = ( 1.0f - alpha) * norm;
    b0 = alpha * norm;
    b1 =   0.0f;
    b2 = -b0;
  }

  void setLSF(float fc, float q, float gain)
  {
    float cos = bqCosOmega(fc);
    float sin = bqSinOmega(fc);
    float A = bqA(gain);
    float beta = bqBeta(gain,q);

    float norm = 1.0f / ((A + 1.0f)+(A - 1.0f) * cos + beta * sin);
    a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cos)) * norm;
    a2 = ((A + 1.0f) + (A - 1.0f) * cos - beta * sin) * norm;
    b0 = ( A * ((A + 1.0f) - (A - 1.0f) * cos + beta * sin)) * norm;
    b1 = ( 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos)) * norm;
    b2 = ( A * ((A + 1.0f) - (A - 1.0f) * cos - beta * sin)) * norm;
  }

  void setHSF(float fc, float q, float gain)
  {
    float cos = bqCosOmega(fc);
    float sin = bqSinOmega(fc);
    float A = bqA(gain);
    float beta = bqBeta(gain,q);

    float norm = 1.0f / ((A + 1.0f) - (A - 1.0f) * cos + beta * sin);
    a1 = ( 2.0f * ((A - 1.0f) - (A + 1.0f) * cos)) * norm;
    a2 = ((A + 1.0f) - (A - 1.0f) * cos - beta * sin) * norm;
    b0 = ( A * ((A + 1.0f) + (A - 1.0f) * cos + beta * sin)) * norm;
    b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos)) * norm;
    b2 = ( A * ((A + 1.0f) + (A - 1.0f) * cos - beta * sin)) * norm;
  }

  void setNF(float fc, float q)
  {
    //float alpha = bqAlphaBW(fc,bw);
    float alpha = bqAlphaQ(fc,q);
    float cos = bqCosOmega(fc);

    float norm = 1.0f / (1.0f + alpha);
    a1 = (-2.0f * cos) * norm;
    a2 = ( 1.0f - alpha) * norm;
    b0 = norm;
    b1 = a1;
    b2 = norm;
  }

  void setAPF(float fc, float q)
  {
    float alpha = bqAlphaQ(fc,q);
    float cos = bqCosOmega(fc);

    float norm = 1.0f / (1.0f + alpha);
    a1 = (-2.0f * cos) * norm;
    a2 = ( 1.0f - alpha) * norm;
    b0 = a2;
    b1 = a1;
    b2 = 1.0f;
  }

};

#endif // LIB_FILTER_HPP
