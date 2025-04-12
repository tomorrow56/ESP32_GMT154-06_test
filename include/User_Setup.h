#define USER_SETUP_INFO "User_Setup"

// Define to use the specific pins for GMT154-06 LCD panel
#define TFT_MISO -1  // Not used for this display
#define TFT_MOSI 23  // DATA pin (IO23)
#define TFT_SCLK 18  // CLK pin (IO18)
#define TFT_CS   -1  // Not used if you're directly connecting to the display
#define TFT_DC    2  // D/C pin (IO2)
#define TFT_RST   4  // RESET pin (IO4)

// Define the backlight pin
#define TFT_BL    25  // Backlight pin (IO25)
#define TFT_BACKLIGHT_ON HIGH  // Adjust as needed based on your LCD module

// Define the display driver
#define ST7789_DRIVER  // ST7789T3 driver for GMT154-06 LCD panel

// ST7789 specific settings
#define TFT_RGB_ORDER TFT_BGR  // BGR順序に変更（赤と青の入れ替わりを修正）
#define SPI_MODE3  // Use SPI mode 3 for ST7789T3

// Define the display resolution
#define TFT_WIDTH  240  // GMT154-06 with ST7789T3 resolution (240x240)
#define TFT_HEIGHT 240  // GMT154-06 with ST7789T3 resolution (240x240)

// Other common settings
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48

#define SMOOTH_FONT

// SPI frequency
#define SPI_FREQUENCY  27000000  // Adjust if you encounter display issues
