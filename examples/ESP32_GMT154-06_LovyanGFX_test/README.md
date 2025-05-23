# ESP32 LovyanGFX Hello World

このプロジェクトはESP32マイコンを使用して、GMT154-06 LCDパネルに「Hello World」を表示するサンプルプログラムです。TFT_eSPIライブラリの代わりにLovyanGFXライブラリを使用しています。

## ハードウェア要件

- ESP32開発ボード
- GMT154-06 LCDパネル（ST7789ドライバ搭載）
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
- LovyanGFX ライブラリ（v1.1.9以上）

## ビルド方法

1. PlatformIOをインストールします
2. このプロジェクトをクローンまたはダウンロードします
3. PlatformIOでプロジェクトを開きます
4. ビルドしてESP32にアップロードします

## 動作説明

起動後、LCDパネルに「Hello World」と接続情報が表示されます。バックライトは50%の輝度に設定されています。

## TFT_eSPIとLovyanGFXの違い

LovyanGFXはTFT_eSPIと比較して以下の利点があります：

1. C++のクラス設計によるオブジェクト指向アプローチ
2. 複数のディスプレイを同時に制御可能
3. より多くのディスプレイドライバに対応
4. 高速な描画処理
5. より柔軟な設定オプション

## ライセンス

MITライセンスの下で公開されています。詳細はLICENSEファイルを参照してください。
