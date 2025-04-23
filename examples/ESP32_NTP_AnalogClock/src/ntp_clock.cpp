#include "ntp_clock.h"
#include <WiFi.h>

// Constructor
NTPClock::NTPClock(lgfx::LGFX_Device* display, const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec)
    : lcd(display), ntpServer(ntpServer), gmtOffset_sec(gmtOffset_sec), daylightOffset_sec(daylightOffset_sec),
      last_hour(-1), last_minute(-1), last_second(-1), timeInitialized(false) {
    
    // Default settings
    clockCenterX = 120;
    clockCenterY = 120;
    clockRadius = 90;
    hourHandLength = 50;
    minHandLength = 70;
    secHandLength = 80;
    
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
            lcd->setTextFont(2);
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
    // Draw the clock background
    lcd->fillScreen(TFT_BLACK);
    lcd->fillCircle(clockCenterX, clockCenterY, clockRadius, clockFaceColor);
    lcd->drawCircle(clockCenterX, clockCenterY, clockRadius, clockBorderColor);
    
    // Draw hour markers
    for (int i = 0; i < 12; i++) {
        float angle = i * 30 * DEG_TO_RAD; // 30 degrees each (360 degrees/12)
        int x1 = clockCenterX + (clockRadius - 15) * sin(angle);
        int y1 = clockCenterY - (clockRadius - 15) * cos(angle);
        int x2 = clockCenterX + clockRadius * sin(angle);
        int y2 = clockCenterY - clockRadius * cos(angle);
        
        // Vary the thickness of hour markers
        if (i % 3 == 0) {
            // Make 3, 6, 9, 12 o'clock positions thicker
            lcd->drawLine(x1, y1, x2, y2, hourMarksColor);
            lcd->drawLine(x1+1, y1, x2+1, y2, hourMarksColor);
            lcd->drawLine(x1, y1+1, x2, y2+1, hourMarksColor);
        } else {
            // Other hour positions
            lcd->drawLine(x1, y1, x2, y2, hourMarksColor);
        }
    }
    
    // Draw center point
    lcd->fillCircle(clockCenterX, clockCenterY, 3, TFT_WHITE);
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
        // For the first drawing, draw all hands
        lcd->drawLine(clockCenterX, clockCenterY, hour_x, hour_y, hourHandColor);
        lcd->drawLine(clockCenterX, clockCenterY, min_x, min_y, minHandColor);
        lcd->drawLine(clockCenterX, clockCenterY, sec_x, sec_y, secHandColor);
    } else {
        // Update flags for each hand
        bool hourUpdated = false;
        bool minUpdated = false;
        bool secUpdated = false;
        
        // Update hour hand only if it has moved
        if (hour != last_hour || minute != last_minute) { // Hour hand is affected by minutes too
            // Erase previous hour hand
            float last_hour_angle = ((last_hour % 12) * 30 + last_minute * 0.5) * DEG_TO_RAD;
            int last_hour_x = clockCenterX + hourHandLength * sin(last_hour_angle);
            int last_hour_y = clockCenterY - hourHandLength * cos(last_hour_angle);
            lcd->drawLine(clockCenterX, clockCenterY, last_hour_x, last_hour_y, clockFaceColor);
            
            hourUpdated = true;
        }
        
        // Update minute hand only if it has moved
        if (minute != last_minute) {
            // Erase previous minute hand
            float last_min_angle = last_minute * 6 * DEG_TO_RAD;
            int last_min_x = clockCenterX + minHandLength * sin(last_min_angle);
            int last_min_y = clockCenterY - minHandLength * cos(last_min_angle);
            lcd->drawLine(clockCenterX, clockCenterY, last_min_x, last_min_y, clockFaceColor);
            
            minUpdated = true;
        }
        
        // Update second hand only if it has moved
        if (second != last_second) {
            // Erase previous second hand
            float last_sec_angle = last_second * 6 * DEG_TO_RAD;
            int last_sec_x = clockCenterX + secHandLength * sin(last_sec_angle);
            int last_sec_y = clockCenterY - secHandLength * cos(last_sec_angle);
            lcd->drawLine(clockCenterX, clockCenterY, last_sec_x, last_sec_y, clockFaceColor);
            
            secUpdated = true;
        }
        
        // Drawing order is important: hour, minute, second
        // Output debug information
        Serial.println("[Drawing hands] Color check:");
        Serial.printf("  Hour hand color: 0x%04X (RED)\n", hourHandColor);
        Serial.printf("  Minute hand color: 0x%04X (YELLOW)\n", minHandColor);
        Serial.printf("  Second hand color: 0x%04X (GREEN)\n", secHandColor);
        
        // Draw hour hand - red
        lcd->drawLine(clockCenterX, clockCenterY, hour_x, hour_y, 0xF800);
        
        // Draw minute hand - yellow
        lcd->drawLine(clockCenterX, clockCenterY, min_x, min_y, 0xFFE0);
        
        // Draw second hand - green
        lcd->drawLine(clockCenterX, clockCenterY, sec_x, sec_y, 0x07E0);
    }
    
    // Redraw the center point
    lcd->fillCircle(clockCenterX, clockCenterY, 3, TFT_WHITE);
    
    // Update digital clock only during initialization or when seconds change
    if (last_second == -1 || second != last_second) {
        char timeStr[9];
        sprintf(timeStr, "%02d:%02d:%02d", hour, minute, second);
        
        // Digital clock background
        lcd->fillRect(clockCenterX - 40, clockCenterY + 40, 80, 20, TFT_BLACK);
        
        // Digital clock text
        lcd->setTextColor(0xFFFF);  // White color
        lcd->setTextSize(1);        // Original size
        lcd->setTextFont(2);        // Original font
        lcd->setCursor(clockCenterX - 35, clockCenterY + 45);
        lcd->print(timeStr);
    }
    
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
        lcd->setTextFont(2);
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
