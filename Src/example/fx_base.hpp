#ifndef FX_BASE_HPP
#define FX_BASE_HPP

// 各エフェクトクラスの基底クラス 純粋仮想関数を含む抽象クラス
class fx_base
{
public:
  virtual void init() = 0;

  virtual void deinit() = 0;

  virtual void setParamStr(uint8_t paramNum) = 0;

  virtual void setParam() = 0;

  virtual void process(float xL[], float xR[]) = 0;
};

#endif // FX_BASE_HPP
