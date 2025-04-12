#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>

// Initialize TFT display
TFT_eSPI tft = TFT_eSPI();

// Backlight control constants
const uint8_t BACKLIGHT_PIN = TFT_BL;
const uint8_t BACKLIGHT_CHANNEL = 0;  // PWM channel for backlight
const uint16_t BACKLIGHT_FREQ = 5000; // PWM frequency in Hz
const uint8_t BACKLIGHT_RESOLUTION = 8; // 8-bit resolution (0-255)
const uint8_t BACKLIGHT_MAX = 255; // Maximum brightness
const uint8_t BACKLIGHT_MIN = 10;  // Minimum brightness (not 0 to avoid complete darkness)
const uint16_t FADE_STEP_MS = 15;   // Milliseconds between fade steps

/**
 * @brief Set backlight brightness using PWM
 * @param brightness Brightness level (0-255)
 */
void setBacklightBrightness(uint8_t brightness) {
  // 極性が逆の場合、値を反転させる
  #if (TFT_BACKLIGHT_ON == LOW)
    brightness = BACKLIGHT_MAX - brightness; // 値を反転
  #endif
  
  ledcWrite(BACKLIGHT_CHANNEL, brightness);
}

/**
 * @brief Fade backlight from current to target brightness
 * @param targetBrightness Target brightness level (0-255)
 * @param fadeTimeMs Total fade time in milliseconds
 */
void fadeBacklight(uint8_t targetBrightness, uint16_t fadeTimeMs) {
  uint8_t currentBrightness = ledcRead(BACKLIGHT_CHANNEL);
  int16_t steps = (fadeTimeMs / FADE_STEP_MS);
  
  if (steps <= 0) {
    steps = 1; // Ensure at least one step
  }
  
  float stepSize = static_cast<float>(targetBrightness - currentBrightness) / steps;
  
  for (int16_t i = 0; i < steps; i++) {
    currentBrightness = static_cast<uint8_t>(currentBrightness + stepSize);
    setBacklightBrightness(currentBrightness);
    delay(FADE_STEP_MS);
  }
  
  // Ensure we reach exactly the target brightness
  setBacklightBrightness(targetBrightness);
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("ESP32 LCD Hello World Example with Backlight Control");
  
  // Configure backlight PWM
  ledcSetup(BACKLIGHT_CHANNEL, BACKLIGHT_FREQ, BACKLIGHT_RESOLUTION);
  ledcAttachPin(BACKLIGHT_PIN, BACKLIGHT_CHANNEL);
  
  // Start with backlight off
  setBacklightBrightness(0);
  
  // Initialize the TFT display
  tft.init();
  tft.setRotation(0); // 0度回転（デフォルトの向き）
  
  // Clear the screen with blue background
  tft.fillScreen(TFT_BLUE); // 青色の背景
  
  // Set text parameters
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLUE); // 白いテキスト、青い背景
  
  // Display welcome message
  tft.setCursor(10, 10);
  tft.setTextFont(4); // Use font 4 (medium size)
  tft.println("Hello World!");
  
  // Display additional information
  tft.setCursor(10, 50);
  tft.setTextFont(2); // Use font 2 (small size)
  tft.println("ESP32 with GMT154-06 LCD");
  
  tft.setCursor(10, 80);
  tft.println("Using TFT_eSPI Library");
  
  // Display connection information
  tft.setCursor(10, 120);
  tft.setTextFont(1); // Use font 1 (smallest size)
  tft.println("LCD Connections:");
  tft.setCursor(10, 140);
  tft.println("RESET: IO4, D/C: IO2");
  tft.setCursor(10, 160);
  tft.println("DATA: IO23, CLK: IO18");
  tft.setCursor(10, 180);
  tft.println("Backlight: IO25");
  
  // Display backlight control information
  tft.setCursor(120, 120);
  tft.println("Backlight Control Demo:");
  tft.setCursor(120, 140);
  tft.println("Fade in/out demonstration");
  
  Serial.println("Setup complete");
  
  // Fade in the backlight
  Serial.println("Fading in backlight...");
  fadeBacklight(BACKLIGHT_MAX, 1000);
}

/**
 * @brief Backlight control demo mode
 */
enum BacklightMode {
  MODE_NORMAL = 0,  // Normal brightness
  MODE_PULSE,       // Pulsing effect
  MODE_FADE_STEPS,  // Step-wise fading
  MODE_MAX          // Number of modes
};

// Current backlight mode
BacklightMode currentMode = MODE_NORMAL;
// Last mode change time
uint32_t lastModeChangeMs = 0;
// Mode duration in milliseconds
const uint32_t MODE_DURATION_MS = 5000;
// For pulse effect
float pulsePhase = 0.0f;

void loop() {
  // バックライトを常に最大輝度に固定
  static bool backlightInitialized = false;
  
  if (!backlightInitialized) {
    // 初回のみ実行
    tft.fillRect(120, 140, 115, 40, TFT_BLUE); // 青色の背景を使用
    tft.setCursor(120, 140);
    tft.println("Backlight: Fixed ON");
    tft.setCursor(120, 160);
    tft.println("Brightness: 50%");
    
    // バックライトを50%の輝度に設定
    uint8_t halfBrightness = BACKLIGHT_MAX / 2;
    setBacklightBrightness(halfBrightness);
    
    backlightInitialized = true;
    Serial.println("Backlight set to 50% brightness");
    Serial.print("Brightness value: ");
    Serial.println(halfBrightness);
  }
  
  // 何もしない - バックライトは固定
  delay(1000);
}
