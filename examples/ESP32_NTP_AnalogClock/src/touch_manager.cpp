#include "touch_manager.h"
#include "ntp_clock.h"

// 静的変数の定義
bool TouchManager::isShowingIPAddress = false;

// コンストラクタ
TouchManager::TouchManager(lgfx::LGFX_Device* display, uint8_t touchPin, uint16_t touchThreshold, uint32_t touchDuration)
    : lcd(display), touchPin(touchPin), touchThreshold(touchThreshold), touchDuration(touchDuration),
      isTouching(false), touchStartTime(0), ntpClock(nullptr) {
    
    // プログレスバーのデフォルト設定
    progressBarX = 20;
    progressBarY = 200;
    progressBarWidth = 200;
    progressBarHeight = 20;
    progressBarColor = TFT_GREEN;
    progressBarBgColor = TFT_DARKGREY;
}

// 初期化
void TouchManager::begin() {
    // タッチピンの初期化
    pinMode(touchPin, INPUT);
    
    Serial.printf("タッチセンサー初期化: ピン=%d, 閾値=%d, 長押し時間=%dms\n", 
                  touchPin, touchThreshold, touchDuration);
}

// タッチセンサーを読み取り、長押し検出
bool TouchManager::checkLongTouch() {
    uint16_t touchValue = getTouchValue();
    uint32_t currentTime = millis();
    
    // タッチ検出
    if (touchValue < touchThreshold) {
        // タッチ開始
        if (!isTouching) {
            isTouching = true;
            touchStartTime = currentTime;
            Serial.printf("タッチ開始: 値=%d, 閾値=%d\n", touchValue, touchThreshold);
        }
        
        // タッチ中
        uint32_t elapsedTime = currentTime - touchStartTime;
        
        // プログレスバーを描画
        drawTouchProgress(elapsedTime);
        
        // 長押し検出
        if (elapsedTime >= touchDuration) {
            Serial.println("長押し検出！");
            resetTouch();
            return true;
        }
    } else {
        // タッチ終了
        if (isTouching) {
            Serial.println("タッチ終了");
            resetTouch();
        }
    }
    
    return false;
}

// タッチ中のプログレスバーを描画
void TouchManager::drawTouchProgress(uint32_t elapsedTime) {
    // プログレスバーの背景
    lcd->fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, progressBarBgColor);
    
    // 進捗率を計算
    float progress = min(1.0f, (float)elapsedTime / touchDuration);
    int progressWidth = progressBarWidth * progress;
    
    // プログレスバーを描画
    lcd->fillRect(progressBarX, progressBarY, progressWidth, progressBarHeight, progressBarColor);
    
    // 進捗率をテキストで表示
    char progressText[10];
    sprintf(progressText, "%d%%", (int)(progress * 100));
    
    lcd->setTextColor(TFT_WHITE);
    lcd->setTextSize(1);
    lcd->setTextFont(2);
    lcd->setCursor(progressBarX + progressBarWidth / 2 - 10, progressBarY + progressBarHeight / 2 - 5);
    lcd->print(progressText);
}

// 短いタッチを検出（タップ）
bool TouchManager::checkShortTouch() {
    uint16_t touchValue = getTouchValue();
    uint32_t currentTime = millis();
    static bool wasTouch = false;
    static uint32_t touchEndTime = 0;
    static bool shortTouchDetected = false;
    static uint32_t lastShortTouchTime = 0;
    
    // 前回の短いタッチから一定時間経過しているか確認（連打防止）
    if (currentTime - lastShortTouchTime < 1000) {
        return false;
    }
    
    // タッチ検出
    if (touchValue < touchThreshold) {
        // タッチ開始
        if (!isTouching) {
            isTouching = true;
            touchStartTime = currentTime;
            Serial.printf("タッチ開始: 値=%d, 閾値=%d\n", touchValue, touchThreshold);
        }
        
        // タッチ中の処理
        uint32_t elapsedTime = currentTime - touchStartTime;
        wasTouch = true;
        
        // 長押しになる前に離された場合は短いタッチと判定しない
        if (elapsedTime >= touchDuration) {
            wasTouch = false; // 長押しになったらリセット
        }
        
        // 長押しになる前に離された場合のみ短いタッチと判定する
        // ここでは判定のみ行い、実際の処理はタッチが離された時に行う
    } else {
        // タッチが離された
        if (isTouching) {
            uint32_t touchDurationTime = currentTime - touchStartTime;
            isTouching = false;
            
            // プログレスバーを消去
            lcd->fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, TFT_BLACK);
            
            // IPアドレス表示中でない場合のみ時計の文字盤を再描画
            if (ntpClock != nullptr && !isShowingIPAddress) {
                ntpClock->drawClockFace();
            }
            
            // 短いタッチの判定（0.1秒以上2秒以下）
            if (wasTouch && touchDurationTime < touchDuration && touchDurationTime >= 100 && touchDurationTime <= 2000) {
                Serial.printf("短いタッチ検出: 時間=%dms\n", touchDurationTime);
                wasTouch = false;
                shortTouchDetected = true;
                touchEndTime = currentTime;
                lastShortTouchTime = currentTime; // 最後のタッチ時間を記録
                
                // プログレスバーを消去
                lcd->fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, TFT_BLACK);
            }
        }
    }
    
    // 短いタッチが検出され、まだ処理されていない場合
    if (shortTouchDetected) {
        shortTouchDetected = false;
        return true;
    }
    
    return false;
}

// タッチ状態をリセット
void TouchManager::resetTouch() {
    isTouching = false;
    touchStartTime = 0;
    
    // プログレスバーを消去
    lcd->fillRect(progressBarX, progressBarY, progressBarWidth, progressBarHeight, TFT_BLACK);
    
    // IPアドレス表示中でない場合のみ時計の文字盤を再描画
    if (ntpClock != nullptr && !isShowingIPAddress) {
        ntpClock->drawClockFace();
    }
}

// 現在のタッチ値を取得
uint16_t TouchManager::getTouchValue() {
    // 複数回読み取って平均値を返すことでノイズを軽減
    const int samples = 5;
    uint32_t total = 0;
    
    for (int i = 0; i < samples; i++) {
        total += touchRead(touchPin);
        delay(1); // 少し間隔を空ける
    }
    
    return total / samples;
}
