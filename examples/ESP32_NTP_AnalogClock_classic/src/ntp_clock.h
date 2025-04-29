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
    
    // 色の定義
    uint32_t clockFaceColor;
    uint32_t clockBorderColor;
    uint32_t hourHandColor;
    uint32_t minHandColor;
    uint32_t secHandColor;
    uint32_t hourMarksColor;
    uint32_t textColor;
    
    // 時計の針の前回位置を保存する変数
    int last_hour;
    int last_minute;
    int last_second;
    
    // 時刻同期フラグ
    bool timeInitialized;
    
    // LCDディスプレイ参照
    lgfx::LGFX_Device* lcd;
    
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
    
    // 時計の針を描画
    void drawClockHands(int hour, int minute, int second);
    
    // 現在時刻を取得して時計を更新
    void updateClock();
    
    // 時刻が初期化されているかを取得
    bool isTimeInitialized() { return timeInitialized; }
    
    // 設定を変更
    void setClockSize(int centerX, int centerY, int radius);
    void setHandLengths(int hour, int min, int sec);
    void setColors(uint32_t face, uint32_t border, uint32_t hourHand, 
                  uint32_t minHand, uint32_t secHand, uint32_t marks, uint32_t text);
};

#endif // NTP_CLOCK_H
