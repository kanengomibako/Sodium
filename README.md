# Sodium
### STM32 Programmable Digital Effects Pedal
STM32マイコンを搭載した、プログラミング可能なデジタルエフェクターです。C言語またはC++で開発を行います。もちろんプログラム書き込み後は単体のエフェクターとして動作可能です。



#### 別途必要なもの
- DC9V（センターマイナス）出力のACアダプターで、0.3A以上の電流出力が可能なもの
- PC（Windows、macOS、Linux いずれも可、64bit推奨）<br/>※ 32bit環境では使用できる開発ソフトが限られます。
- STM32対応のデバッガー（プログラム書き込みやデバッグを行う機器）<br/>
	→ [ST-LINK/V2](https://www.st.com/ja/development-tools/st-link-v2.html)、[STM32 Nucleo Board](https://www.st.com/ja/evaluation-tools/stm32-nucleo-boards.html) 等
- PCとデバッガーを接続するUSBケーブル
- デバッガーとSodium本体を接続するケーブル<br/>
	→ [コネクタ付コード 3P](https://akizukidenshi.com/catalog/g/gC-15384/) 等



#### 各部の機能
- INPUT 端子：ギターや他の機器からの出力を接続します。
- OUTPUT 端子：アンプや他の機器の入力へ接続します。
- +9VDC 端子：ACアダプターを接続します。
- 各スイッチ：任意の機能をプログラミング可能です。初期プログラムでの動作は、別ページへ
- Serial Wire Debug (SWD)接続ピン：デバッガーを接続します。接続の詳細は、別ページへ



| 主な仕様 |  |
| - | - |
| サンプリング周波数 | 44.1 kHz（48 kHz または 96 kHz に設定可能） |
| 内部演算 | 32ビット浮動小数点 |
| AD、DA変換 | 24 ビット |
| レイテンシ | 1.6 ミリ秒（初期プログラム実行時） |
| 最大入力レベル | 1.0 Vrms (+2.2 dBu) |
| 入力インピーダンス | 1 MΩ |
| 最大出力レベル | 1.35 Vrms (+4.8 dBu) |
| 出力インピーダンス | 1 kΩ |
| バイパス | バッファード・バイパス（バイパス音もAD/DA変換されます） |
| コントロール | 押しボタンスイッチ×4　フットスイッチ×1 |
| ディスプレイ | 有機ELディスプレイ　モノクロ128×64ドット |
| インジケーター | 3色LED（直径5mm） |
| 接続端子 | INPUT端子　OUTPUT端子　電源入力端子<br/>Serial Wire Debug (SWD)接続ピン |
| 消費電流 | 150 mA |
| 外形寸法 | 幅 73 mm × 奥行 107 mm × 高さ 48 mm<br/>（フットスイッチ部を除いた高さ 35 mm） |
| 質量 | 240 g |

