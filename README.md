# ESP32 LCD Hello World

このプロジェクトはESP32マイコンを使用して、GMT154-06 LCDパネルに「Hello World」を表示するサンプルプログラムです。

## ハードウェア要件

- ESP32開発ボード
- GMT154-06 LCDパネル
- 接続ワイヤー

## 接続方法

LCDパネルとESP32の接続は以下の通りです：

| LCD ピン | ESP32 ピン |
|---------|-----------|
| RESET   | IO4       |
| D/C     | IO2       |
| DATA    | IO23      |
| CLK     | IO18      |
| バックライト | IO25      |

## ソフトウェア要件

- PlatformIO
- Arduino Framework for ESP32
- TFT_eSPI ライブラリ

## ビルド方法

1. PlatformIOをインストールします
2. このプロジェクトをクローンまたはダウンロードします
3. PlatformIOでプロジェクトを開きます
4. ビルドしてESP32にアップロードします

## 動作説明

起動後、LCDパネルに「Hello World」と接続情報が表示されます。
