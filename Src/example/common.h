#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <string>
using std::string;

/* 各定数設定 --------------------------*/

// ペダル名称表示
#define PEDAL_NAME "Sodium v0.3"

// ブロックサイズ まとめて処理を行う数
#define BLOCK_SIZE 16

// 円周率
#define PI 3.14159265359f

// サンプリング周波数
#define SAMPLING_FREQ 44108.07f

// スイッチ短押し時間 ミリ秒
#define SHORT_PUSH_MSEC 20

// スイッチ長押し時間 ミリ秒
#define LONG_PUSH_MSEC 1000

// ステータス情報表示時間 ミリ秒
#define STATUS_DISP_MSEC 1000

// 最大エフェクト数
#define MAX_FX_NUM 3

// エフェクト番号割当
enum FXtype {OD, DD, TR};

// 最小値、最大値、絶対値関数
#ifndef min
#define min(x, a) ((x) < (a) ? (x) : (a))
#endif
#ifndef max
#define max(x, b) ((x) > (b) ? (x) : (b))
#endif
#ifndef abs
#define abs(x) ((x) > 0 ? (x) : -(x))
#endif

// 最小値a、最大値bでクリップ
#define clip(x, a, b) (((x) < (a) ? (a) : (x)) > (b) ? (b) : ((x) < (a) ? (a) : (x)))

/* グローバル変数 --------------------------*/

// user_main.cpp で定義
extern bool sw[];
extern int16_t fxParam[];
extern int16_t fxParamMax[];
extern int16_t fxParamMin[];
extern string fxParamName[];
extern string fxParamStr[];
extern uint8_t fxNum;
extern int16_t fxAllData[MAX_FX_NUM][20];
extern uint8_t fxParamIndexMax;

// fx.cpp で定義
extern string fxNameList[MAX_FX_NUM];
extern uint16_t fxColorList[MAX_FX_NUM];

#endif // COMMON_H
