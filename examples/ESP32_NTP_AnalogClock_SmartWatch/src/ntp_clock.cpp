#include "ntp_clock.h"
#include <WiFi.h>
#include "clock_face.h" // 時計文字盤ビットマップデータ

// 文字盤が描画済みかチェックするフラグ
static bool clockFaceDrawn = false;

// ビットマップを使用して時計文字盤をLCDに描画する関数のプロトタイプ
void drawClockFaceBitmapToLCD(lgfx::LGFX_Device* lcd, int centerX, int centerY, int radius);

// Constructor
NTPClock::NTPClock(lgfx::LGFX_Device* display, const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec)
    : lcd(display), ntpServer(ntpServer), gmtOffset_sec(gmtOffset_sec), daylightOffset_sec(daylightOffset_sec),
      last_hour(-1), last_minute(-1), last_second(-1), last_day(-1), last_wday(-1), timeInitialized(false),
      last_sec_x1(0), last_sec_y1(0), last_sec_startX(0), last_sec_startY(0),
      last_min_x1(0), last_min_y1(0), last_min_startX(0), last_min_startY(0),
      last_hour_x1(0), last_hour_y1(0), last_hour_startX(0), last_hour_startY(0),
      backgroundSprite(nullptr), clockSprite(nullptr), backgroundInitialized(false), clockSpriteInitialized(false) {
    
    // Default settings - スマートウォッチ風のサイズに調整
    clockCenterX = 120; // 240x240の中心
    clockCenterY = 120;
    clockRadius = 110;
    hourHandLength = 60;
    minHandLength = 80;
    secHandLength = 95;
    centerAvoidRadius = 6; // 針の中心部分の半径
    
    // スマートウォッチ風のカラー設定 - RGB565形式で直接定義
    clockFaceColor = TFT_BLACK;     // 文字盤：黒
    clockBorderColor = 0xFD00;      // 外側リング：赤みのオレンジ（RGB565形式）
    hourHandColor = 0xFB00;         // 時針：赤みのオレンジ（RGB565形式）
    minHandColor = 0xFB00;          // 分針：赤みのオレンジ（RGB565形式）
    secHandColor = 0xF800;          // 秒針：赤（RGB565形式）
    hourMarksColor = TFT_WHITE;     // 時間マーカー：白
    textColor = 0xFD00;             // テキスト：赤みのオレンジ（RGB565形式）
    
    // スプライトの初期化はコンストラクタでは行わない
    // begin関数で行う
}

// Initialize
void NTPClock::begin() {
    // 既存のスプライトをクリーンアップ
    if (backgroundSprite != nullptr) {
        backgroundSprite->deleteSprite();
        delete backgroundSprite;
        backgroundSprite = nullptr;
    }
    
    if (clockSprite != nullptr) {
        clockSprite->deleteSprite();
        delete clockSprite;
        clockSprite = nullptr;
    }
    
    backgroundInitialized = false;
    clockSpriteInitialized = false;
    
    // 画面をクリア
    lcd->fillScreen(TFT_BLACK);
    
    // 背景用スプライトを初期化
    backgroundSprite = new LGFX_Sprite(lcd);
    if (backgroundSprite) {
        backgroundSprite->setColorDepth(16); // 16ビットカラーモード（RGB565）
        backgroundSprite->setPsram(false);   // PSRAM不使用で高速化
        
        // 背景スプライトのサイズを設定
        if (!backgroundSprite->createSprite(240, 240)) {
            delete backgroundSprite;
            backgroundSprite = nullptr;
            Serial.println("Failed to allocate memory for background sprite");
            return;
        }
        
        // 背景スプライトに文字盤を描画
        backgroundSprite->fillScreen(TFT_BLACK);
        drawClockFaceBitmapToSprite(backgroundSprite, clockCenterX, clockCenterY, clockRadius);
        
        backgroundInitialized = true;
        Serial.println("Background sprite initialized successfully");
    } else {
        Serial.println("Failed to create background sprite");
        return;
    }
    
    // ダブルバッファリング用のスプライトを初期化
    clockSprite = new LGFX_Sprite(lcd);
    if (clockSprite) {
        clockSprite->setColorDepth(16); // 16ビットカラーモード（RGB565）
        clockSprite->setPsram(false);   // PSRAM不使用で高速化
        
        // 時計スプライトのサイズを設定
        if (!clockSprite->createSprite(240, 240)) {
            delete clockSprite;
            clockSprite = nullptr;
            Serial.println("Failed to allocate memory for clock sprite");
            return;
        }
        
        clockSpriteInitialized = true;
        Serial.println("Clock sprite initialized successfully");
    } else {
        Serial.println("Failed to create clock sprite");
        return;
    }
    
    // 文字盤の初期化
    clockFaceDrawn = false;
    
    // 文字盤描画フラグをリセット
    clockFaceDrawn = false;
}

// 背景スプライトに文字盤を描画する関数
void NTPClock::drawClockFaceBitmapToSprite(LGFX_Sprite* sprite, int centerX, int centerY, int radius) {
    // ビットマップデータのサイズを取得
    int imgWidth = ::imgWidth;
    int imgHeight = ::imgHeight;
    
    // ビットマップの左上の座標を計算
    int startX = centerX - imgWidth / 2;
    int startY = centerY - imgHeight / 2;
    
    // ビットマップデータをピクセル単位で描画
    int index = 0;
    for (int y = 0; y < imgHeight; y++) {
        for (int x = 0; x < imgWidth; x++) {
            // PROGMEMから色データを読み出し
            uint16_t color = pgm_read_word(&img[index++]);
            
            // バイトオーダーを調整（必要に応じて）
            color = ((color & 0x00FF) << 8) | ((color & 0xFF00) >> 8);
            
            // ピクセルを描画
            sprite->drawPixel(startX + x, startY + y, color);
        }
    }
}

// Get time from NTP server
bool NTPClock::syncTimeWithNTP() {
    if (WiFi.status() == WL_CONNECTED) {
        // シリアルにのみ同期開始メッセージを表示
        Serial.println("Synchronizing with NTP server...");
        
        // 初回のみLCDに同期メッセージを表示
        if (!timeInitialized) {
            lcd->fillScreen(TFT_BLACK);
            lcd->setTextColor(TFT_WHITE);
            lcd->setTextSize(1);
            lcd->setFont(&fonts::Font2);
            lcd->setCursor(10, 10);
            lcd->println("Synchronizing with NTP server...");
        }
        
        // NTPサーバーと時刻同期
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        
        // 時刻取得を試みる
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            Serial.println("Time synchronization successful!");
            
            // 初回のみLCDに成功メッセージを表示
            if (!timeInitialized) {
                lcd->setCursor(10, 30);
                lcd->println("Time sync OK");
                
                char timeStr[50];
                strftime(timeStr, sizeof(timeStr), "%Y/%m/%d %H:%M:%S", &timeinfo);
                lcd->setCursor(10, 50);
                lcd->println(timeStr);
                
                delay(2000);
            }
            
            // 初回のみ文字盤を再描画、それ以外は現在の表示を維持
            bool firstSync = !timeInitialized;
            timeInitialized = true;
            
            if (firstSync) {
                drawClockFace();
            }
            
            return true;
        } else {
            Serial.println("Time synchronization failed");
            
            // 初回のみLCDに失敗メッセージを表示
            if (!timeInitialized) {
                lcd->setCursor(10, 30);
                lcd->println("Time sync failed");
                lcd->setCursor(10, 50);
                lcd->println("Using internal clock");
                delay(2000);
                drawClockFace();
            }
            
            return false;
        }
    }
    return false;
}

// ビットマップを使用して時計文字盤をLCDに描画する関数
// 画像データの変換元ツール: https://lang-ship.com/tools/image2data/
void drawClockFaceBitmapToLCD(lgfx::LGFX_Device* lcd, int centerX, int centerY, int radius) {
    // ビットマップの寸法（240x240ピクセル）
    // clock_face.hから直接値を使用
    
    // ビットマップを中央に配置するための開始位置を計算
    int startX = centerX - imgWidth / 2;
    int startY = centerY - imgHeight / 2;
    
    // 画面外にはみ出さないように調整
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    
    Serial.println("Drawing 240x240 bitmap image to display");
    
    // ビットマップデータを直接LCDに描画
    // img配列から240x240のRGB565ビットマップデータを読み込んで描画
    int index = 0;
    for (int y = 0; y < imgHeight; y++) {
        for (int x = 0; x < imgWidth; x++) {
            // PROGMEMからデータを読み込む
            uint16_t color = pgm_read_word(&img[index++]);
            
            // RGB565のバイトオーダーを調整（必要に応じて）
            // オレンジ色が青色になっている場合、赤と青のチャンネルが入れ替わっている可能性がある
            // バイトオーダーを入れ替える
            color = ((color & 0x00FF) << 8) | ((color & 0xFF00) >> 8);
            
            // 画面内の場合のみ描画
            if ((startX + x) < lcd->width() && (startY + y) < lcd->height()) {
                lcd->drawPixel(startX + x, startY + y, color);
            }
        }
    }
    
    Serial.println("Bitmap drawing complete");
}

// Draw the clock face - SmartWatch style
void NTPClock::drawClockFace() {
    if (clockFaceDrawn) {
        return; // 既に描画済みなら何もしない
    }
    
    // 背景スプライトが利用可能な場合は、スプライトを表示
    if (backgroundInitialized && backgroundSprite != nullptr) {
        lcd->startWrite();
        backgroundSprite->pushSprite(0, 0);
        lcd->endWrite();
    } else {
        // スプライトが使えない場合は直接描画
        drawClockFaceBitmapToLCD(lcd, clockCenterX, clockCenterY, clockRadius);
    }
    
    clockFaceDrawn = true;
}

// 画面全体を再描画する
void NTPClock::redrawFullScreen() {
    // ダブルバッファリング用のスプライトが初期化されていない場合は従来の方法で再描画
    if (!clockSpriteInitialized || !backgroundInitialized || clockSprite == nullptr || backgroundSprite == nullptr) {
        // 文字盤描画フラグをリセットして強制再描画
        clockFaceDrawn = false;
        
        // 画面をクリア
        lcd->fillScreen(TFT_BLACK);
        
        // 文字盤を再描画
        if (backgroundInitialized && backgroundSprite != nullptr) {
            lcd->startWrite();
            backgroundSprite->pushSprite(0, 0);
            lcd->endWrite();
            clockFaceDrawn = true;
        } else {
            drawClockFace();
        }
        
        // 現在時刻を取得して針を再描画
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            // 針の前回位置をリセットして強制再描画
            last_hour = -1;
            last_minute = -1;
            last_second = -1;
            
            // 針を強制描画
            drawClockHands(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        return;
    }
    
    // ダブルバッファリングを使用した再描画
    // 現在時刻を取得
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        // 針の前回位置をリセットして強制再描画
        last_hour = -1;
        last_minute = -1;
        last_second = -1;
        
        // ステップ1: 背景スプライトを時計スプライトにコピー
        backgroundSprite->pushSprite(clockSprite, 0, 0);
        
        // ステップ2: 時計スプライトに針と日付を描画
        
        // 時針を描画
        float hour_angle = ((timeinfo.tm_hour % 12) * 30 + timeinfo.tm_min * 0.5) * DEG_TO_RAD;
        int hour_x1 = clockCenterX + hourHandLength * sin(hour_angle);
        int hour_y1 = clockCenterY - hourHandLength * cos(hour_angle);
        int hour_startX = clockCenterX + centerAvoidRadius * sin(hour_angle);
        int hour_startY = clockCenterY - centerAvoidRadius * cos(hour_angle);
        
        for (int dx = -3; dx <= 3; ++dx) {
            clockSprite->drawLine(hour_startX+dx, hour_startY, hour_x1+dx, hour_y1, hourHandColor);
        }
        
        // 分針を描画
        float min_angle = timeinfo.tm_min * 6 * DEG_TO_RAD;
        int min_x1 = clockCenterX + minHandLength * sin(min_angle);
        int min_y1 = clockCenterY - minHandLength * cos(min_angle);
        int min_startX = clockCenterX + centerAvoidRadius * sin(min_angle);
        int min_startY = clockCenterY - centerAvoidRadius * cos(min_angle);
        
        for (int dx = -2; dx <= 2; ++dx) {
            clockSprite->drawLine(min_startX+dx, min_startY, min_x1+dx, min_y1, minHandColor);
        }
        
        // 秒針を描画
        float sec_angle = timeinfo.tm_sec * 6 * DEG_TO_RAD;
        int sec_x1 = clockCenterX + secHandLength * sin(sec_angle);
        int sec_y1 = clockCenterY - secHandLength * cos(sec_angle);
        int sec_startX = clockCenterX + centerAvoidRadius * sin(sec_angle);
        int sec_startY = clockCenterY - centerAvoidRadius * cos(sec_angle);
        
        clockSprite->drawLine(sec_startX, sec_startY, sec_x1, sec_y1, secHandColor);
        
        // 日付と曜日を描画
        char dayBuf[8];
        static const char* const dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        sprintf(dayBuf, "%s %d", dayNames[timeinfo.tm_wday], timeinfo.tm_mday);
        
        clockSprite->setTextColor(0xFFFF); // RGB565形式で白色を指定
        clockSprite->setFont(&fonts::Font2);
        int dayWidth = clockSprite->textWidth(dayBuf);
        clockSprite->setCursor(clockCenterX - dayWidth/2, clockCenterY - 40);
        clockSprite->print(dayBuf);
        
        // 中心点を描画
        clockSprite->fillCircle(clockCenterX, clockCenterY, 4, 0xFFFF); // RGB565形式で白色を指定
        
        // ステップ3: 時計スプライトをLCDに描画
        lcd->startWrite();
        clockSprite->pushSprite(lcd, 0, 0);
        lcd->endWrite();
        
        // 現在の針の位置を記憶
        last_hour = timeinfo.tm_hour;
        last_minute = timeinfo.tm_min;
        last_second = timeinfo.tm_sec;
        
        // 秒針の位置を記憶
        last_sec_x1 = sec_x1;
        last_sec_y1 = sec_y1;
        last_sec_startX = sec_startX;
        last_sec_startY = sec_startY;
        
        // 分針の位置を記憶
        last_min_x1 = min_x1;
        last_min_y1 = min_y1;
        last_min_startX = min_startX;
        last_min_startY = min_startY;
        
        // 時針の位置を記憶
        last_hour_x1 = hour_x1;
        last_hour_y1 = hour_y1;
        last_hour_startX = hour_startX;
        last_hour_startY = hour_startY;
    }
}

// 文字盤描画フラグをリセット
void NTPClock::resetClockFaceDrawnFlag() {
    clockFaceDrawn = false;
}

// 背景スプライトの一部を再表示
void NTPClock::restoreBackgroundArea(int x, int y, int width, int height) {
    // ダブルバッファリング方式では、部分再描画は不要
    // 互換性のために関数は残しておく
    
    // ダブルバッファリングが有効な場合は、全体再描画を行う
    if (clockSpriteInitialized && clockSprite != nullptr) {
        // 現在時刻を取得
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            // 時計の針を再描画
            drawClockHands(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        return;
    }
    
    // 旧式の方法（ダブルバッファリングが無効な場合のフォールバック）
    if (backgroundInitialized && backgroundSprite != nullptr) {
        // 座標が画面内に収まるように調整
        if (x < 0) {
            width += x;
            x = 0;
        }
        if (y < 0) {
            height += y;
            y = 0;
        }
        if (x + width > 240) width = 240 - x;
        if (y + height > 240) height = 240 - y;
        
        // 有効な領域がある場合のみ描画
        if (width > 0 && height > 0) {
            lcd->startWrite();
            lcd->setClipRect(x, y, width, height);
            backgroundSprite->pushSprite(0, 0);
            lcd->clearClipRect();
            lcd->endWrite();
        }
    } else {
        // 背景スプライトがない場合は単に背景色で塗りつぶす
        lcd->startWrite();
        lcd->fillRect(x, y, width, height, clockFaceColor);
        lcd->endWrite();
    }
}

// Draw the clock hands
void NTPClock::drawClockHands(int hour, int minute, int second) {
    // ダブルバッファリング用のスプライトが初期化されていない場合は従来の方法で描画
    if (!clockSpriteInitialized || !backgroundInitialized || clockSprite == nullptr || backgroundSprite == nullptr) {
        // 従来の方法で描画
        if (!clockFaceDrawn) {
            drawClockFace();
        }
        
        // 更新項目を判定
        bool hourChanged = (last_hour != hour);
        bool minuteChanged = (last_minute != minute);
        bool secondChanged = (last_second != second);
        
        // 日付の変更を確認
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        bool dateChanged = (last_day != timeinfo.tm_mday || last_wday != timeinfo.tm_wday);
        if (dateChanged) {
            last_day = timeinfo.tm_mday;
            last_wday = timeinfo.tm_wday;
        }
        
        // 何か変化があれば再描画
        if (hourChanged || minuteChanged || secondChanged || dateChanged) {
            // 前回の針を消去
            if (secondChanged && last_second >= 0) {
                // 秒針の前回位置を消去
                restoreBackgroundArea(last_sec_startX - 1, last_sec_startY - 1, 
                                    abs(last_sec_x1 - last_sec_startX) + 3, 
                                    abs(last_sec_y1 - last_sec_startY) + 3);
            }
            
            if (minuteChanged && last_minute >= 0) {
                // 分針の前回位置を消去
                restoreBackgroundArea(last_min_startX - 3, last_min_startY - 3, 
                                    abs(last_min_x1 - last_min_startX) + 7, 
                                    abs(last_min_y1 - last_min_startY) + 7);
            }
            
            if (hourChanged && last_hour >= 0) {
                // 時針の前回位置を消去
                restoreBackgroundArea(last_hour_startX - 4, last_hour_startY - 4, 
                                    abs(last_hour_x1 - last_hour_startX) + 9, 
                                    abs(last_hour_y1 - last_hour_startY) + 9);
            }
            
            // 日付と曜日を描画
            if (dateChanged) {
                char dayBuf[8];
                static const char* const dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
                sprintf(dayBuf, "%s %d", dayNames[timeinfo.tm_wday], timeinfo.tm_mday);
                
                lcd->setTextColor(0xFFFF); // RGB565形式で白色を指定
                lcd->setFont(&fonts::Font2);
                int dayWidth = lcd->textWidth(dayBuf);
                lcd->setCursor(clockCenterX - dayWidth/2, clockCenterY - 40);
                lcd->print(dayBuf);
            }
            
            // 時針を描画
            float hour_angle = ((hour % 12) * 30 + minute * 0.5) * DEG_TO_RAD;
            int hour_x1 = clockCenterX + hourHandLength * sin(hour_angle);
            int hour_y1 = clockCenterY - hourHandLength * cos(hour_angle);
            int hour_startX = clockCenterX + centerAvoidRadius * sin(hour_angle);
            int hour_startY = clockCenterY - centerAvoidRadius * cos(hour_angle);
            
            for (int dx = -3; dx <= 3; ++dx) {
                lcd->drawLine(hour_startX+dx, hour_startY, hour_x1+dx, hour_y1, hourHandColor);
            }
            
            // 分針を描画
            float min_angle = minute * 6 * DEG_TO_RAD;
            int min_x1 = clockCenterX + minHandLength * sin(min_angle);
            int min_y1 = clockCenterY - minHandLength * cos(min_angle);
            int min_startX = clockCenterX + centerAvoidRadius * sin(min_angle);
            int min_startY = clockCenterY - centerAvoidRadius * cos(min_angle);
            
            for (int dx = -2; dx <= 2; ++dx) {
                lcd->drawLine(min_startX+dx, min_startY, min_x1+dx, min_y1, minHandColor);
            }
            
            // 秒針を描画
            float sec_angle = second * 6 * DEG_TO_RAD;
            int sec_x1 = clockCenterX + secHandLength * sin(sec_angle);
            int sec_y1 = clockCenterY - secHandLength * cos(sec_angle);
            int sec_startX = clockCenterX + centerAvoidRadius * sin(sec_angle);
            int sec_startY = clockCenterY - centerAvoidRadius * cos(sec_angle);
            
            lcd->drawLine(sec_startX, sec_startY, sec_x1, sec_y1, secHandColor);
            
            // 中心点を描画
            lcd->fillCircle(clockCenterX, clockCenterY, 4, 0xFFFF); // RGB565形式で白色を指定
            
            // 現在の針の位置を記憶
            last_hour = hour;
            last_minute = minute;
            last_second = second;
            
            // 秒針の位置を記憶
            last_sec_x1 = sec_x1;
            last_sec_y1 = sec_y1;
            last_sec_startX = sec_startX;
            last_sec_startY = sec_startY;
            
            // 分針の位置を記憶
            last_min_x1 = min_x1;
            last_min_y1 = min_y1;
            last_min_startX = min_startX;
            last_min_startY = min_startY;
            
            // 時針の位置を記憶
            last_hour_x1 = hour_x1;
            last_hour_y1 = hour_y1;
            last_hour_startX = hour_startX;
            last_hour_startY = hour_startY;
        }
        return;
    }
    
    // 更新項目を判定
    bool hourChanged = (last_hour != hour);
    bool minuteChanged = (last_minute != minute);
    bool secondChanged = (last_second != second);
    bool dateChanged = false;
    
    // 日付の変更を確認
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    if (last_day != timeinfo.tm_mday || last_wday != timeinfo.tm_wday) {
        dateChanged = true;
        last_day = timeinfo.tm_mday;
        last_wday = timeinfo.tm_wday;
    }
    
    // 何か変化があれば再描画
    if (hourChanged || minuteChanged || secondChanged || dateChanged || !clockFaceDrawn) {
        // ステップ1: 背景スプライトを時計スプライトにコピー
        backgroundSprite->pushSprite(clockSprite, 0, 0);
        
        // ステップ2: 時計スプライトに針を描画
        
        // 日付と曜日を描画
        char dayBuf[8];
        static const char* const dayNames[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        sprintf(dayBuf, "%s %d", dayNames[timeinfo.tm_wday], timeinfo.tm_mday);
        
        clockSprite->setTextColor(0xFFFF); // RGB565形式で白色を指定
        clockSprite->setFont(&fonts::Font2);
        int dayWidth = clockSprite->textWidth(dayBuf);
        clockSprite->setCursor(clockCenterX - dayWidth/2, clockCenterY - 40);
        clockSprite->print(dayBuf);
        
        // 時針を描画
        float hour_angle = ((hour % 12) * 30 + minute * 0.5) * DEG_TO_RAD;
        int hour_x1 = clockCenterX + hourHandLength * sin(hour_angle);
        int hour_y1 = clockCenterY - hourHandLength * cos(hour_angle);
        int hour_startX = clockCenterX + centerAvoidRadius * sin(hour_angle);
        int hour_startY = clockCenterY - centerAvoidRadius * cos(hour_angle);
        
        for (int dx = -3; dx <= 3; ++dx) {
            clockSprite->drawLine(hour_startX+dx, hour_startY, hour_x1+dx, hour_y1, hourHandColor);
        }
        
        // 分針を描画
        float min_angle = minute * 6 * DEG_TO_RAD;
        int min_x1 = clockCenterX + minHandLength * sin(min_angle);
        int min_y1 = clockCenterY - minHandLength * cos(min_angle);
        int min_startX = clockCenterX + centerAvoidRadius * sin(min_angle);
        int min_startY = clockCenterY - centerAvoidRadius * cos(min_angle);
        
        for (int dx = -2; dx <= 2; ++dx) {
            clockSprite->drawLine(min_startX+dx, min_startY, min_x1+dx, min_y1, minHandColor);
        }
        
        // 秒針を描画
        float sec_angle = second * 6 * DEG_TO_RAD;
        int sec_x1 = clockCenterX + secHandLength * sin(sec_angle);
        int sec_y1 = clockCenterY - secHandLength * cos(sec_angle);
        int sec_startX = clockCenterX + centerAvoidRadius * sin(sec_angle);
        int sec_startY = clockCenterY - centerAvoidRadius * cos(sec_angle);
        
        clockSprite->drawLine(sec_startX, sec_startY, sec_x1, sec_y1, secHandColor);
        
        // 中心点を描画
        clockSprite->fillCircle(clockCenterX, clockCenterY, 4, 0xFFFF); // RGB565形式で白色を指定
        
        // ステップ3: 時計スプライトをLCDに描画
        lcd->startWrite();
        clockSprite->pushSprite(lcd, 0, 0);
        lcd->endWrite();
        
        // 前回の針の位置を記憶
        last_sec_x1 = sec_x1;
        last_sec_y1 = sec_y1;
        last_sec_startX = sec_startX;
        last_sec_startY = sec_startY;
        
        last_min_x1 = min_x1;
        last_min_y1 = min_y1;
        last_min_startX = min_startX;
        last_min_startY = min_startY;
        
        last_hour_x1 = hour_x1;
        last_hour_y1 = hour_y1;
        last_hour_startX = hour_startX;
        last_hour_startY = hour_startY;
        
        // 時間を更新
        last_hour = hour;
        last_minute = minute;
        last_second = second;
        
        // 文字盤描画フラグをセット
        clockFaceDrawn = true;
    }
}

// Get current time and update the clock
void NTPClock::updateClock() {
    struct tm timeinfo;
    
    if (getLocalTime(&timeinfo)) {
        // Successfully retrieved time from NTP
        drawClockHands(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else if (timeInitialized) {
        // Failed to get time from NTP, but previously synchronized
        // Use internal clock
        time_t now = time(nullptr);
        localtime_r(&now, &timeinfo);
        drawClockHands(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        // Time not initialized, display error message
        // ダブルバッファリングが有効な場合
        if (clockSpriteInitialized && clockSprite != nullptr) {
            // エラー表示用にスプライトをクリア
            clockSprite->fillScreen(TFT_BLACK);
            
            // エラーメッセージをスプライトに描画
            clockSprite->setTextColor(TFT_RED);
            clockSprite->setTextSize(1);
            clockSprite->setFont(&fonts::Font2);
            clockSprite->setCursor(10, 10);
            clockSprite->println("Time not set");
            clockSprite->setCursor(10, 30);
            clockSprite->println("Please check WiFi connection");
            
            // スプライトをLCDに転送
            lcd->startWrite();
            clockSprite->pushSprite(lcd, 0, 0);
            lcd->endWrite();
        } else {
            // 従来の方法（ダブルバッファリングが無効な場合）
            // 文字盤を再描画する必要がある
            clockFaceDrawn = false;
            
            // エラー表示用に画面をクリア
            lcd->fillScreen(TFT_BLACK);
            
            // エラーメッセージを直接LCDに描画
            lcd->setTextColor(TFT_RED);
            lcd->setTextSize(1);
            lcd->setFont(&fonts::Font2);
            lcd->setCursor(10, 10);
            lcd->println("Time not set");
            lcd->setCursor(10, 30);
            lcd->println("Please check WiFi connection");
        }
    }
}

// Set clock size
void NTPClock::setClockSize(int centerX, int centerY, int radius) {
    clockCenterX = centerX;
    clockCenterY = centerY;
    clockRadius = radius;
}

// Set hand lengths
void NTPClock::setHandLengths(int hour, int min, int sec) {
    hourHandLength = hour;
    minHandLength = min;
    secHandLength = sec;
}

// Set colors (RGB565形式の16ビットカラー)
void NTPClock::setColors(uint16_t face, uint16_t border, uint16_t hourHand, 
                         uint16_t minHand, uint16_t secHand, uint16_t marks, uint16_t text) {
    clockFaceColor = face;
    clockBorderColor = border;
    hourHandColor = hourHand;
    minHandColor = minHand;
    secHandColor = secHand;
    hourMarksColor = marks;
    textColor = text;
}
