#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <string>
using std::string;

/* 各定数設定 --------------------------*/

// ペダル名称表示
#define PEDAL_NAME "Sodium v1.1"

// ブロックサイズ まとめて処理を行う数
#define BLOCK_SIZE 16

// サンプリング周波数
#define SAMPLING_FREQ 44108.07f

// スイッチ短押し時間 ミリ秒
#define SHORT_PUSH_MSEC 20

// スイッチ長押し時間 ミリ秒
#define LONG_PUSH_MSEC 1000

// ステータス情報表示時間 ミリ秒
#define STATUS_DISP_MSEC 1000

// タップテンポ機能 有効1 無効0
#define TAP_ENABLED 1

// タップテンポ最大時間 ミリ秒
#define MAX_TAP_TIME 3000.0f

// チューナー機能 有効1 無効0
#define TUNER_ENABLED 1

// 入出力クリップ検出機能 有効1 無効0
#define CLIP_DETECT_ENABLED 0

// データ保存先 セクターと開始アドレス
#define DATA_SECTOR FLASH_SECTOR_5
#define DATA_ADDR 0x08020000

/* 関数 -------------------------------------*/

// 円周率
#define PI 3.14159265359f

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

// fx.cpp で定義
extern const uint8_t fxNumMax;
extern int16_t fxAllData[][20];

// user_main.cpp で定義
extern bool fxOn;
extern string fxName;
extern uint16_t fxColor;
extern int16_t fxParam[];
extern int16_t fxParamMax[];
extern int16_t fxParamMin[];
extern string fxParamName[];
extern string fxParamStr[];
extern uint8_t fxParamNumMax;
extern uint8_t fxNum;
extern float tapTime;

/* LED色定義 -----------------------------*/

#define COLOR_RGB 0b1111111111111111; // 赤緑青（白）
#define COLOR_RG  0b1111111111100000; // 赤緑（黄）
#define COLOR_BG  0b0000011111111111; // 青緑
#define COLOR_RB  0b1111100000011111; // 赤青（紫）
#define COLOR_R   0b1111100000000000; // 赤
#define COLOR_G   0b0000000000011111; // 緑
#define COLOR_B   0b0000000000011111; // 青

#endif // COMMON_H
