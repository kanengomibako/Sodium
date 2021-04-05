#include "fx.hpp"

// 基底クラス ヘッダー
#include "fx_base.hpp"

// 各エフェクト 派生クラス ヘッダー
#include "fx_boost.hpp"
#include "fx_overdrive.hpp"
#include "fx_delay.hpp"
#include "fx_tremolo.hpp"
#include "fx_chorus.hpp"
#include "fx_phaser.hpp"
#include "fx_reverb.hpp"

// 各エフェクト グローバルインスタンス生成
static fx_boost bo1;
static fx_overdrive od1;
static fx_delay dd1;
static fx_tremolo tr1;
static fx_chorus ce1;
static fx_phaser ph1;
static fx_reverb rv1;

// 各エフェクト用ポインタ配列を生成(&を付ける)
// 使用するエフェクトを記載、順番入れ替え可
static fx_base* effect[] = {&bo1, &od1, &dd1, &tr1, &ce1, &ph1, &rv1};


const uint8_t fxNumMax = sizeof(effect) / sizeof(effect[0]); // エフェクト最大数を自動計算

uint32_t cpuUsageCycleMax[fxNumMax] = {}; // CPU使用サイクル数 各エフェクトごとに最大値を記録
int16_t fxAllData[fxNumMax][20] = {}; // 全てのエフェクトパラメータデータ配列

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

// 終了処理 ディレイ用メモリ削除等
void fxDeinit()
{
  effect[fxNum]->deinit();
}

// エフェクトパラメータ文字列更新処理
void fxSetParamStr(uint8_t paramNum)
{
  effect[fxNum]->setParamStr(paramNum);
}
