#include "ntp_clock.h"
#include <WiFi.h>

// Constructor
NTPClock::NTPClock(lgfx::LGFX_Device* display, const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec)
    : lcd(display), ntpServer(ntpServer), gmtOffset_sec(gmtOffset_sec), daylightOffset_sec(daylightOffset_sec),
      last_hour(-1), last_minute(-1), last_second(-1), timeInitialized(false) {
    
    // Default settings - 画面全体を使用するサイズに調整
    clockCenterX = 120; // 240x240の中心
    clockCenterY = 120; // 240x240の中心
    clockRadius = 112; // 3ドット分小さく調整
    hourHandLength = 63; // 時針の長さも比例して調整
    minHandLength = 83; // 分針の長さも比例して調整
    secHandLength = 93; // 秒針の長さも比例して調整
    
    // Default color settings - using direct constants for clarity
    clockFaceColor = TFT_NAVY;
    clockBorderColor = TFT_WHITE;
    hourHandColor = 0xF800;        // Hour hand: red (0xF800)
    minHandColor = 0xFFE0;         // Minute hand: yellow (0xFFE0)
    secHandColor = 0x07E0;         // Second hand: green (0x07E0)
    hourMarksColor = 0xFFFF;       // Hour markers: white (0xFFFF)
    textColor = 0xFFFF;            // Text: white (0xFFFF)
}

// Initialize
void NTPClock::begin() {
    // Draw the clock face
    // drawClockFace();  // 起動時の自動描画を抑止（UI制御のため）
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

// Draw the clock face
void NTPClock::drawClockFace() {
    // 1. 背景と枠（太線で描画）
    lcd->fillScreen(TFT_WHITE);
    int borderWidth = 4; // 枠の太さ
    lcd->fillCircle(clockCenterX, clockCenterY, clockRadius, TFT_BLACK); // 黒で外側描画
    lcd->fillCircle(clockCenterX, clockCenterY, clockRadius - borderWidth, TFT_WHITE); // 白で内側描画

    // 2. 60分割分目盛り
    for (int i = 0; i < 60; ++i) {
        float angle = i * 6.0f * DEG_TO_RAD;
        int len = (i % 5 == 0) ? 14 : 7; // 時目盛りは長く
        int x1 = clockCenterX + (clockRadius - len) * sin(angle);
        int y1 = clockCenterY - (clockRadius - len) * cos(angle);
        int x2 = clockCenterX + (clockRadius - 2) * sin(angle);
        int y2 = clockCenterY - (clockRadius - 2) * cos(angle);
        if (i % 5 == 0) {
            // 太い線で時目盛り
            lcd->drawLine(x1, y1, x2, y2, TFT_BLACK);
            lcd->drawLine(x1+1, y1, x2+1, y2, TFT_BLACK);
        } else {
            // 分目盛り
            lcd->drawLine(x1, y1, x2, y2, TFT_BLACK);
        }
    }
    // 3. 数字（大きめに調整）
    lcd->setTextColor(TFT_BLACK, TFT_WHITE);
    lcd->setFont(&fonts::Font4); // 大きめフォントに変更
    for (int i = 1; i <= 12; ++i) {
        float angle = (i - 3) * 30.0f * DEG_TO_RAD;
        // 数字の位置を外側に移動して大きいフォントに対応
        int tx = clockCenterX + (clockRadius - 30) * cos(angle);
        int ty = clockCenterY + (clockRadius - 30) * sin(angle);
        char buf[3];
        sprintf(buf, "%d", i);
        int w = lcd->textWidth(buf);
        int h = lcd->fontHeight();
        lcd->setCursor(tx - w/2, ty - h/2);
        lcd->print(buf);
    }
    // 4. 中心点（黒で大きめ）
    lcd->fillCircle(clockCenterX, clockCenterY, 6, TFT_BLACK);
}

// Draw the clock hands
void NTPClock::drawClockHands(int hour, int minute, int second) {
    // Calculate second hand angle (60 seconds per rotation)
    float sec_angle = second * 6 * DEG_TO_RAD;
    int sec_x = clockCenterX + secHandLength * sin(sec_angle);
    int sec_y = clockCenterY - secHandLength * cos(sec_angle);
    
    // Calculate minute hand angle (60 minutes per rotation)
    float min_angle = minute * 6 * DEG_TO_RAD;
    int min_x = clockCenterX + minHandLength * sin(min_angle);
    int min_y = clockCenterY - minHandLength * cos(min_angle);
    
    // Calculate hour hand angle (12 hours per rotation, considering minutes)
    float hour_angle = ((hour % 12) * 30 + minute * 0.5) * DEG_TO_RAD;
    int hour_x = clockCenterX + hourHandLength * sin(hour_angle);
    int hour_y = clockCenterY - hourHandLength * cos(hour_angle);
    
    // Draw all hands during initialization
    if (last_hour == -1 && last_minute == -1 && last_second == -1) {
        // 逆側の長さ（針ごとに調整可）
        int hourBack = 18, minBack = 22, secBack = 26;
        int centerAvoidRadius = 8; // 中心円を避ける半径
        
        // 時針（太さ倍増、中心部分は避ける）
        float hour_angle = ((hour % 12) * 30 + minute * 0.5) * DEG_TO_RAD;
        int hour_x1 = clockCenterX + hourHandLength * sin(hour_angle);
        int hour_y1 = clockCenterY - hourHandLength * cos(hour_angle);
        int hour_x2 = clockCenterX - hourBack * sin(hour_angle);
        int hour_y2 = clockCenterY + hourBack * cos(hour_angle);
        for (int dx = -2; dx <= 2; ++dx) { // 太さ倍増
            // 中心円を避けて描画
            int startX1 = clockCenterX + centerAvoidRadius * sin(hour_angle);
            int startY1 = clockCenterY - centerAvoidRadius * cos(hour_angle);
            int startX2 = clockCenterX - centerAvoidRadius * sin(hour_angle);
            int startY2 = clockCenterY + centerAvoidRadius * cos(hour_angle);
            lcd->drawLine(startX2+dx, startY2, hour_x2+dx, hour_y2, TFT_BLACK);
            lcd->drawLine(startX1+dx, startY1, hour_x1+dx, hour_y1, TFT_BLACK);
        }
        
        // 分針（太さ倍増、中心部分は避ける）
        float min_angle = minute * 6 * DEG_TO_RAD;
        int min_x1 = clockCenterX + minHandLength * sin(min_angle);
        int min_y1 = clockCenterY - minHandLength * cos(min_angle);
        int min_x2 = clockCenterX - minBack * sin(min_angle);
        int min_y2 = clockCenterY + minBack * cos(min_angle);
        for (int dx = -1; dx <= 1; ++dx) { // 太さ倍増
            // 中心円を避けて描画
            int startX1 = clockCenterX + centerAvoidRadius * sin(min_angle);
            int startY1 = clockCenterY - centerAvoidRadius * cos(min_angle);
            int startX2 = clockCenterX - centerAvoidRadius * sin(min_angle);
            int startY2 = clockCenterY + centerAvoidRadius * cos(min_angle);
            lcd->drawLine(startX2+dx, startY2, min_x2+dx, min_y2, TFT_BLACK);
            lcd->drawLine(startX1+dx, startY1, min_x1+dx, min_y1, TFT_BLACK);
        }
        
        // 秒針（中心部分は避ける）
        float sec_angle = second * 6 * DEG_TO_RAD;
        int sec_x1 = clockCenterX + secHandLength * sin(sec_angle);
        int sec_y1 = clockCenterY - secHandLength * cos(sec_angle);
        int sec_x2 = clockCenterX - secBack * sin(sec_angle);
        int sec_y2 = clockCenterY + secBack * cos(sec_angle);
        // 中心円を避けて描画
        int startX1 = clockCenterX + centerAvoidRadius * sin(sec_angle);
        int startY1 = clockCenterY - centerAvoidRadius * cos(sec_angle);
        int startX2 = clockCenterX - centerAvoidRadius * sin(sec_angle);
        int startY2 = clockCenterY + centerAvoidRadius * cos(sec_angle);
        lcd->drawLine(startX2, startY2, sec_x2, sec_y2, TFT_RED);
        lcd->drawLine(startX1, startY1, sec_x1, sec_y1, TFT_RED);
    } else {
        // Update flags for each hand
        bool hourUpdated = false;
        bool minUpdated = false;
        bool secUpdated = false;
        
        // 中心円を避ける半径
        int centerAvoidRadius = 8;
        
        // Update hour hand only if it has moved
        if (hour != last_hour || minute != last_minute) { // Hour hand is affected by minutes too
            // Erase previous hour hand（太さ倍増、逆側も消去、中心部分は避ける）
            float last_hour_angle = ((last_hour % 12) * 30 + last_minute * 0.5) * DEG_TO_RAD;
            int last_hour_x1 = clockCenterX + hourHandLength * sin(last_hour_angle);
            int last_hour_y1 = clockCenterY - hourHandLength * cos(last_hour_angle);
            int hourBack = 18;
            int last_hour_x2 = clockCenterX - hourBack * sin(last_hour_angle);
            int last_hour_y2 = clockCenterY + hourBack * cos(last_hour_angle);
            for (int dx = -2; dx <= 2; ++dx) { // 太さ倍増
                // 中心円を避けて描画
                int startX1 = clockCenterX + centerAvoidRadius * sin(last_hour_angle);
                int startY1 = clockCenterY - centerAvoidRadius * cos(last_hour_angle);
                int startX2 = clockCenterX - centerAvoidRadius * sin(last_hour_angle);
                int startY2 = clockCenterY + centerAvoidRadius * cos(last_hour_angle);
                lcd->drawLine(startX2+dx, startY2, last_hour_x2+dx, last_hour_y2, TFT_WHITE);
                lcd->drawLine(startX1+dx, startY1, last_hour_x1+dx, last_hour_y1, TFT_WHITE);
            }
            hourUpdated = true;
        }
        
        // Update minute hand only if it has moved
        if (minute != last_minute) {
            // Erase previous minute hand（太さ倍増、逆側も消去、中心部分は避ける）
            float last_min_angle = last_minute * 6 * DEG_TO_RAD;
            int last_min_x1 = clockCenterX + minHandLength * sin(last_min_angle);
            int last_min_y1 = clockCenterY - minHandLength * cos(last_min_angle);
            int minBack = 22;
            int last_min_x2 = clockCenterX - minBack * sin(last_min_angle);
            int last_min_y2 = clockCenterY + minBack * cos(last_min_angle);
            for (int dx = -1; dx <= 1; ++dx) { // 太さ倍増
                // 中心円を避けて描画
                int startX1 = clockCenterX + centerAvoidRadius * sin(last_min_angle);
                int startY1 = clockCenterY - centerAvoidRadius * cos(last_min_angle);
                int startX2 = clockCenterX - centerAvoidRadius * sin(last_min_angle);
                int startY2 = clockCenterY + centerAvoidRadius * cos(last_min_angle);
                lcd->drawLine(startX2+dx, startY2, last_min_x2+dx, last_min_y2, TFT_WHITE);
                lcd->drawLine(startX1+dx, startY1, last_min_x1+dx, last_min_y1, TFT_WHITE);
            }
            minUpdated = true;
        }
        
        // Update second hand only if it has moved
        if (second != last_second) {
            // Erase previous second hand（逆側も消去、中心部分は避ける）
            float last_sec_angle = last_second * 6 * DEG_TO_RAD;
            int last_sec_x1 = clockCenterX + secHandLength * sin(last_sec_angle);
            int last_sec_y1 = clockCenterY - secHandLength * cos(last_sec_angle);
            int secBack = 26;
            int last_sec_x2 = clockCenterX - secBack * sin(last_sec_angle);
            int last_sec_y2 = clockCenterY + secBack * cos(last_sec_angle);
            // 中心円を避けて描画
            int startX1 = clockCenterX + centerAvoidRadius * sin(last_sec_angle);
            int startY1 = clockCenterY - centerAvoidRadius * cos(last_sec_angle);
            int startX2 = clockCenterX - centerAvoidRadius * sin(last_sec_angle);
            int startY2 = clockCenterY + centerAvoidRadius * cos(last_sec_angle);
            // 秒針の軌跡を背景色（白）で消す（両端描画）
            lcd->drawLine(startX2, startY2, last_sec_x2, last_sec_y2, TFT_WHITE);
            lcd->drawLine(startX1, startY1, last_sec_x1, last_sec_y1, TFT_WHITE);
            secUpdated = true;
        }
        
        // Drawing order is important: hour, minute, second
        // Output debug information
        Serial.println("[Drawing hands] Color check:");
        Serial.printf("  Hour hand color: 0x%04X (RED)\n", hourHandColor);
        Serial.printf("  Minute hand color: 0x%04X (YELLOW)\n", minHandColor);
        Serial.printf("  Second hand color: 0x%04X (GREEN)\n", secHandColor);
        
        // 逆側の長さ（針ごとに調整可）
        int hourBack = 18, minBack = 22, secBack = 26;
        
        // 時針（太さ倍増、中心部分は避ける）
        float hour_angle = ((hour % 12) * 30 + minute * 0.5) * DEG_TO_RAD;
        int hour_x1 = clockCenterX + hourHandLength * sin(hour_angle);
        int hour_y1 = clockCenterY - hourHandLength * cos(hour_angle);
        int hour_x2 = clockCenterX - hourBack * sin(hour_angle);
        int hour_y2 = clockCenterY + hourBack * cos(hour_angle);
        for (int dx = -2; dx <= 2; ++dx) { // 太さ倍増
            // 中心円を避けて描画
            int startX1 = clockCenterX + centerAvoidRadius * sin(hour_angle);
            int startY1 = clockCenterY - centerAvoidRadius * cos(hour_angle);
            int startX2 = clockCenterX - centerAvoidRadius * sin(hour_angle);
            int startY2 = clockCenterY + centerAvoidRadius * cos(hour_angle);
            lcd->drawLine(startX2+dx, startY2, hour_x2+dx, hour_y2, TFT_BLACK);
            lcd->drawLine(startX1+dx, startY1, hour_x1+dx, hour_y1, TFT_BLACK);
        }
        
        // 分針（太さ倍増、中心部分は避ける）
        float min_angle = minute * 6 * DEG_TO_RAD;
        int min_x1 = clockCenterX + minHandLength * sin(min_angle);
        int min_y1 = clockCenterY - minHandLength * cos(min_angle);
        int min_x2 = clockCenterX - minBack * sin(min_angle);
        int min_y2 = clockCenterY + minBack * cos(min_angle);
        for (int dx = -1; dx <= 1; ++dx) { // 太さ倍増
            // 中心円を避けて描画
            int startX1 = clockCenterX + centerAvoidRadius * sin(min_angle);
            int startY1 = clockCenterY - centerAvoidRadius * cos(min_angle);
            int startX2 = clockCenterX - centerAvoidRadius * sin(min_angle);
            int startY2 = clockCenterY + centerAvoidRadius * cos(min_angle);
            lcd->drawLine(startX2+dx, startY2, min_x2+dx, min_y2, TFT_BLACK);
            lcd->drawLine(startX1+dx, startY1, min_x1+dx, min_y1, TFT_BLACK);
        }
        
        // 秒針（中心部分は避ける）
        float sec_angle = second * 6 * DEG_TO_RAD;
        int sec_x1 = clockCenterX + secHandLength * sin(sec_angle);
        int sec_y1 = clockCenterY - secHandLength * cos(sec_angle);
        int sec_x2 = clockCenterX - secBack * sin(sec_angle);
        int sec_y2 = clockCenterY + secBack * cos(sec_angle);
        // 中心円を避けて描画
        int startX1 = clockCenterX + centerAvoidRadius * sin(sec_angle);
        int startY1 = clockCenterY - centerAvoidRadius * cos(sec_angle);
        int startX2 = clockCenterX - centerAvoidRadius * sin(sec_angle);
        int startY2 = clockCenterY + centerAvoidRadius * cos(sec_angle);
        lcd->drawLine(startX2, startY2, sec_x2, sec_y2, TFT_RED);
        lcd->drawLine(startX1, startY1, sec_x1, sec_y1, TFT_RED);
    }
    
    // Redraw the center point（白い円＋黒縁取り＋中心黒ドット）サイズ半分
    lcd->fillCircle(clockCenterX, clockCenterY, 5, TFT_WHITE); // 白円（半分）
    lcd->drawCircle(clockCenterX, clockCenterY, 5, TFT_BLACK); // 黒縁（半分）
    lcd->fillCircle(clockCenterX, clockCenterY, 2, TFT_BLACK);  // 中心黒ドット（半分）
    
    // Update digital clock only during initialization or when seconds change
    // if (last_second == -1 || second != last_second) {
    //     char timeStr[9];
    //     sprintf(timeStr, "%02d:%02d:%02d", hour, minute, second);
    //     
    //     // Digital clock background
    //     lcd->fillRect(clockCenterX - 40, clockCenterY + 40, 80, 20, TFT_BLACK);
    //     
    //     // Digital clock text
    //     lcd->setTextColor(0xFFFF);  // White color
    //     lcd->setTextSize(1);        // Original size
    //     lcd->setFont(&fonts::Font2);        // Original font
    //     lcd->setCursor(clockCenterX - 35, clockCenterY + 45);
    //     lcd->print(timeStr);
    // }
    
    // Save current hand positions
    last_hour = hour;
    last_minute = minute;
    last_second = second;
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
        lcd->fillScreen(TFT_BLACK);
        lcd->setTextColor(TFT_RED);
        lcd->setTextSize(1);
        lcd->setFont(&fonts::Font2);
        lcd->setCursor(10, 10);
        lcd->println("Time not set");
        lcd->setCursor(10, 30);
        lcd->println("Please check WiFi connection");
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

// Set colors
void NTPClock::setColors(uint32_t face, uint32_t border, uint32_t hourHand, 
                        uint32_t minHand, uint32_t secHand, uint32_t marks, uint32_t text) {
    clockFaceColor = face;
    clockBorderColor = border;
    hourHandColor = hourHand;
    minHandColor = minHand;
    secHandColor = secHand;
    hourMarksColor = marks;
    textColor = text;
}
