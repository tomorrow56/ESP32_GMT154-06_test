#ifndef NTP_CLOCK_H
#define NTP_CLOCK_H

#include <Arduino.h>
#include <time.h>
#include <LovyanGFX.hpp>

class NTPClock {
private:
    // NTP設定
    const char* ntpServer;
    long gmtOffset_sec;
    int daylightOffset_sec;
    
    // 時計の設定
    int clockCenterX;
    int clockCenterY;
    int clockRadius;
    int hourHandLength;
    int minHandLength;
    int secHandLength;
    int centerAvoidRadius;  // 針の中心部分の半径
    
    // 色の定義（RGB565形式の16ビットカラー）
    uint16_t clockFaceColor;
    uint16_t clockBorderColor;
    uint16_t hourHandColor;
    uint16_t minHandColor;
    uint16_t secHandColor;
    uint16_t hourMarksColor;
    uint16_t textColor;
    
    // 時計の針と日付の前回位置を保存する変数
    int last_hour;
    int last_minute;
    int last_second;
    int last_day;    // 日付
    int last_wday;   // 曜日
    
    // 秒針の前回の位置を記憶するための変数
    int last_sec_x1;
    int last_sec_y1;
    int last_sec_startX;
    int last_sec_startY;
    
    // 分針の前回の位置を記憶するための変数
    int last_min_x1;
    int last_min_y1;
    int last_min_startX;
    int last_min_startY;
    
    // 時針の前回の位置を記憶するための変数
    int last_hour_x1;
    int last_hour_y1;
    int last_hour_startX;
    int last_hour_startY;
    
    // 時刻同期フラグ
    bool timeInitialized;
    
    // LCDディスプレイ参照
    lgfx::LGFX_Device* lcd;
    
    // 背景用のスプライト
    LGFX_Sprite* backgroundSprite;
    
    // ダブルバッファリング用のスプライト
    LGFX_Sprite* clockSprite;       // 時計全体用スプライト
    
    // スプライト初期化フラグ
    bool backgroundInitialized;
    bool clockSpriteInitialized;
    
public:
    // コンストラクタ
    NTPClock(lgfx::LGFX_Device* display, 
             const char* ntpServer = "ntp.nict.jp",
             long gmtOffset_sec = 9 * 3600,
             int daylightOffset_sec = 0);
    
    // 初期化
    void begin();
    
    // NTPサーバーから時刻を取得
    bool syncTimeWithNTP();
    
    // 時計の文字盤を描画
    void drawClockFace();
    
    // 背景スプライトに文字盤を描画
    void drawClockFaceBitmapToSprite(LGFX_Sprite* sprite, int centerX, int centerY, int radius);
    
    // 背景スプライトの一部を再表示
    void restoreBackgroundArea(int x, int y, int width, int height);
    
    // 画面全体を再描画する
    void redrawFullScreen();
    
    // 文字盤描画フラグをリセット
    void resetClockFaceDrawnFlag();
    
    // 時計の針を描画
    void drawClockHands(int hour, int minute, int second);
    
    // 現在時刻を取得して時計を更新
    void updateClock();
    
    // 時刻が初期化されているかを取得
    bool isTimeInitialized() { return timeInitialized; }
    
    // 設定を変更
    void setClockSize(int centerX, int centerY, int radius);
    void setHandLengths(int hour, int min, int sec);
    void setColors(uint16_t face, uint16_t border, uint16_t hourHand, 
                   uint16_t minHand, uint16_t secHand, uint16_t marks, uint16_t text);
};

#endif // NTP_CLOCK_H
