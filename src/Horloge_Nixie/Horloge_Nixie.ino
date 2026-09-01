
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <simpleRTC.h> // pin 28 : SCL - pin 27 SDA - sur uC SCL : A5 - SDA : A4

#define BUFFPIXEL 20

#define TFT_CS        2
#define TFT_CS2       5
#define TFT_CS3       6
#define TFT_CS4       7
#define TFT_RST      -1
#define TFT_DC        9
#define SD_CS         4
//D11 -> MOSI SD + SDA TFT
//D12 -> MISO SD
//D13 -> SCK SD + SCL TFT

constexpr  uint8_t dizaine(uint8_t x) {
  return  ((x) / 10);
}
constexpr  uint8_t unite(uint8_t x) {
  return ((x) % 10);
}

unsigned long tempsAffichage;
boolean ledState = false;

static const char heures[29][8] PROGMEM = { // H pour les heures - M pour les minutes - D = dizaine - U = unité
  "HD0.bmp",
  "HD1.bmp",
  "HD2.bmp",
  "HU0.bmp",
  "HU1.bmp",
  "HU2.bmp",
  "HU3.bmp",
  "HU4.bmp",
  "HU5.bmp",
  "HU6.bmp",
  "HU7.bmp",
  "HU8.bmp",
  "HU9.bmp",
  "MD0.bmp",
  "MD1.bmp",
  "MD2.bmp",
  "MD3.bmp",
  "MD4.bmp",
  "MD5.bmp",
  "MU0.bmp",
  "MU1.bmp",
  "MU2.bmp",
  "MU3.bmp",
  "MU4.bmp",
  "MU5.bmp",
  "MU6.bmp",
  "MU7.bmp",
  "MU8.bmp",
  "MU9.bmp"
};

// instances de la classe Adafruit_ST7735
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_ST7735 tft2 = Adafruit_ST7735(TFT_CS2, TFT_DC, TFT_RST);
Adafruit_ST7735 tft3 = Adafruit_ST7735(TFT_CS3, TFT_DC, TFT_RST);
Adafruit_ST7735 tft4 = Adafruit_ST7735(TFT_CS4, TFT_DC, TFT_RST);



void setup() {
  pinMode(A0, OUTPUT);
  pinMode(A1 , OUTPUT);
  Wire.begin();
  Serial.begin(115200);


  tft.initR(INITR_BLACKTAB);
  tft2.initR(INITR_BLACKTAB);
  tft3.initR(INITR_BLACKTAB);
  tft4.initR(INITR_BLACKTAB);

  tft.initR(INITR_MINI160x80_PLUGIN);
  tft2.initR(INITR_MINI160x80_PLUGIN);
  tft3.initR(INITR_MINI160x80_PLUGIN);
  tft4.initR(INITR_MINI160x80_PLUGIN);

  if (!SD.begin(SD_CS)) {
    return;
  }
  Serial.println("OK!");

  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  tft2.setRotation(0);
  tft2.fillScreen(ST77XX_BLACK);
  tft3.setRotation(0);
  tft3.fillScreen(ST77XX_BLACK);
  tft4.setRotation(0);
  tft4.fillScreen(ST77XX_BLACK);
  AffichageHeure();
}

void loop() {

  uint8_t heureDePlus = RTC.heure() + 1;
  uint8_t minuteDePlus = RTC.minute() + 1;

  RTC.actualiser();

  if (millis() - tempsAffichage > 1000) {
    ledState = !ledState;
    digitalWrite(A0, ledState);
    digitalWrite(A1, ledState);
    tempsAffichage = millis();
  }

  if (RTC.heure() == heureDePlus || (RTC.heure() == 0 && RTC.minute() == 0 && RTC.seconde() == 0)) {
    uint8_t h = RTC.heure();
    for (uint8_t x = 0 ; x < 3 ; x++)  { // Heures (dizaine)
      if (dizaine(h) == x) bmpDraw(tft, (__FlashStringHelper*)  heures[x], 0, 0);
    }
    for (uint8_t x = 0 ; x < 10 ; x++)  { // Heures (unités)
      if (unite(h) == x) bmpDraw(tft2, (__FlashStringHelper*)  heures[3 + x], 0, 0);
    }
    heureDePlus = RTC.heure();
  }
  if (RTC.minute() == minuteDePlus || (RTC.minute() == 0 && RTC.seconde() == 0)) {
    uint8_t m = RTC.minute();
    for (uint8_t x = 0 ; x < 6 ; x++)  {// Minutes (dizaine)
      if (dizaine(m) == x) bmpDraw(tft3, (__FlashStringHelper*)  heures[13 + x], 0, 0);
    }
    for (uint8_t x = 0 ; x < 10 ; x++)  { // Minutes (unités)
      if (unite(m) == x) bmpDraw(tft4, (__FlashStringHelper*)  heures[19 + x], 0, 0);
    }
    minuteDePlus = RTC.minute();
  }
}
// Cette fonction ouvre un fichier Windows Bitmap (BMP) et
// l'affiche aux coordonnées données.  C'est accéléré
// en lisant plusieurs pixels de données à la fois
// (plutôt que pixel par pixel).  Augmenter le tampon
// la taille prend plus de la précieuse RAM de l'Arduino mais
// rend le chargement un peu plus rapide.  20 pixels semblent un
// bon équilibre.

void bmpDraw(Adafruit_ST7735 & tft,  __FlashStringHelper * filename, int8_t x, int16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0; //startTime = millis();

  if ((x >= tft.width()) || (y >= tft.height())) return;

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == false) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      //Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!


        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width())  w = tft.width()  - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x + w - 1, y + h - 1);

        for (row = 0; row < h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col = 0; col < w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];

            tft.pushColor(tft.color565(r, g, b));
          } // end pixel
        } // end scanline
      } // end goodBmp
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void AffichageHeure() {
  RTC.actualiser();
  uint8_t h = RTC.heure();
  uint8_t m = RTC.minute();

  for (uint8_t x = 0 ; x < 3 ; x++)  {
    if (dizaine(h) == x) bmpDraw(tft, (__FlashStringHelper*)  heures[x], 0, 0);
  }
  for (uint8_t x = 0 ; x < 10 ; x++)  {
    if (unite(h) == x) bmpDraw(tft2, (__FlashStringHelper*)  heures[3 + x], 0, 0);
  }
  for (uint8_t x = 0 ; x < 6 ; x++)  {
    if (dizaine(m) == x) bmpDraw(tft3, (__FlashStringHelper*)  heures[13 + x], 0, 0);
  }
  for (uint8_t x = 0 ; x < 10 ; x++)  {
    if (unite(m) == x) bmpDraw(tft4, (__FlashStringHelper*)  heures[19 + x], 0, 0);
  }
}
