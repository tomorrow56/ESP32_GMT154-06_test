#ifndef TOUCH_MANAGER_H
#define TOUCH_MANAGER_H

#include <Arduino.h>
#include <LovyanGFX.hpp>

// 前方宣言
class NTPClock;

class TouchManager {
private:
    // タッチセンサー設定
    uint8_t touchPin;
    uint16_t touchThreshold;
    uint32_t touchDuration;
    
    // タッチ状態
    bool isTouching;
    uint32_t touchStartTime;
    

    
    // LCDディスプレイ参照
    lgfx::LGFX_Device* lcd;
    
    // NTPClock参照
    NTPClock* ntpClock;
    
    // プログレスバー設定
    int progressBarX;
    int progressBarY;
    int progressBarWidth;
    int progressBarHeight;
    uint32_t progressBarColor;
    uint32_t progressBarBgColor;
    
public:
    // IPアドレス表示中かどうかを示すフラグ
    static bool isShowingIPAddress;
    
    // コンストラクタ
    TouchManager(lgfx::LGFX_Device* display, 
                uint8_t touchPin = 39, 
                uint16_t touchThreshold = 83, 
                uint32_t touchDuration = 3000);
    
    // NTPClockを設定
    void setNTPClock(NTPClock* clock) { ntpClock = clock; }
    
    // 初期化
    void begin();
    
    // タッチセンサーを読み取り、長押し検出
    bool checkLongTouch();
    
    // 短いタッチを検出（タップ）
    bool checkShortTouch();
    
    // タッチ中のプログレスバーを描画
    void drawTouchProgress(uint32_t elapsedTime);
    
    // タッチ状態をリセット
    void resetTouch();
    
    // 現在のタッチ値を取得
    uint16_t getTouchValue();
    
    // 設定を変更
    void setTouchThreshold(uint16_t threshold) { touchThreshold = threshold; }
    uint16_t getTouchThreshold() const { return touchThreshold; }
    void setTouchDuration(uint32_t duration) { touchDuration = duration; }
    void setProgressBarPosition(int x, int y) { progressBarX = x; progressBarY = y; }
    void setProgressBarSize(int width, int height) { progressBarWidth = width; progressBarHeight = height; }
    void setProgressBarColors(uint32_t color, uint32_t bgColor) { progressBarColor = color; progressBarBgColor = bgColor; }
};

#endif // TOUCH_MANAGER_H
