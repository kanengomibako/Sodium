#include "fx.hpp"

string fxNameList[MAX_FX_NUM] = {}; // エフェクト名のリスト
uint16_t fxColorList[MAX_FX_NUM] = {};   // エフェクトLED色 RGB565

// 基底クラス ヘッダー
#include "fx_base.hpp"

// 各エフェクト 派生クラス ヘッダー
#include "fx_overdrive.hpp"
#include "fx_delay.hpp"
#include "fx_tremolo.hpp"
#include "fx_chorus.hpp"
#include "fx_phaser.hpp"
#include "fx_reverb.hpp"

// 各エフェクト グローバルインスタンス生成
static fx_overdrive od1;
static fx_delay dd1;
static fx_tremolo tr1;
static fx_chorus ce1;
static fx_phaser ph1;
static fx_reverb rv1;

// 各エフェクト用ポインタ配列を生成
static fx_base* effect[MAX_FX_NUM] = {&od1, &dd1, &tr1, &ce1, &ph1, &rv1};

// エフェクト処理
void fxProcess(float xL[], float xR[])
{
  effect[fxNum]->process(xL, xR);
}

// 初期化処理 パラメータ読込、ディレイ用メモリ確保等
void fxInit()
{
  effect[fxNum]->init();
}

// 終了処理 ディレイ用メモリ縮小等
void fxDeinit()
{
  effect[fxNum]->deinit();
}

// エフェクトパラメータ文字列更新処理
void fxSetParamStr(uint8_t paramNum)
{
  effect[fxNum]->setParamStr(paramNum);
}
