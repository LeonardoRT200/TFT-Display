#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <FS.h>
#include "SPIFFS.h"


// Include SPIFFS
#define FS_NO_GLOBALS

// Comment out to stop drawing white spots
#define WHITE_SPOT

// This is the file name used to store the touch coordinate
// calibration data. Change the name to start a new calibration.
#define CALIBRATION_FILE "/TouchCalData3"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

//Dimensions and coordinates of Button get 
#define BTN_GET_X 205
#define BTN_GET_Y 100
#define COR_BTN_GET_X 60
#define COR_BTN_GET_Y 300
#define LAB_COR_BTN_GET_X 70
#define LAB_COR_BTN_GET_Y 345

//Dimensions and coordinates of Button Recalibrate
#define BTN_RECALI_X 150
#define BTN_RECALI_Y 30
#define COR_BTN_RECALI_X 150
#define COR_BTN_RECALI_Y 450
#define LAB_COR_BTN_RECALI_X 165
#define LAB_COR_BTN_RECALI_Y 460

// Load tabs attached to this sketch
#include "List_SPIFFS.h"
#include "Web_Fetch.h"

#include <ezButton.h>

//const char* ssid = "INFINITUM05A4_2.4";
//const char* password = "EHr3bHaKy3";

const char* ssid = "DESKTOP-UOUOP00 0082";
const char* password = "91k136#M";

ezButton button(14);  // create ezButton object that attach to pin 7;

String camServer = "http://192.168.137.146";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

// Initialize the TFT screen
TFT_eSPI tft = TFT_eSPI();

// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_SKYBLUE, TFT_BLACK, 15);
    tft.fillScreen(TFT_BLACK);

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

void setup() {
  Serial.begin(9600);
  button.setDebounceTime(1);
  tft.begin();
  tft.setRotation(0);
  
  // call screen calibration
  touch_calibrate();
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);

  // The byte order can be swapped (set true for TFT_eSPI)
  TJpgDec.setSwapBytes(true);

  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
  tft.println("Iniciando...");
  delay(1000);
  tft.println("Conectando...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.println("Conectando...");
  }
  tft.println("Conectado a:");
  tft.println(ssid);
  tft.println("Creando SPIFFS...");
  if (!SPIFFS.begin(true)) {
    tft.println("Error al crear SPIFFS");
    return;
  } else {
    tft.println("SPIFFS creado correctamente");
  }
  delay(1000);
}

void loop() {
   tft.drawRect(COR_BTN_GET_X,COR_BTN_GET_Y,BTN_GET_X,BTN_GET_Y,TFT_WHITE);
   tft.drawString("Capturar Imagen ", LAB_COR_BTN_GET_X , LAB_COR_BTN_GET_Y);
   tft.drawRect(COR_BTN_RECALI_X,COR_BTN_RECALI_Y,BTN_RECALI_X,BTN_RECALI_Y,TFT_WHITE);
   tft.drawString("Recalibrar ", LAB_COR_BTN_RECALI_X , LAB_COR_BTN_RECALI_Y);
   
   uint16_t x, y;

  if (tft.getTouch(&x, &y))
  {
    
    // Draw a white spot to show where touch was calculated to be
    #ifdef WHITE_SPOT
      tft.fillCircle(x, y, 4, TFT_WHITE);
        Serial.println("Estas tocando en");
        Serial.println(x);
        Serial.println(y);
    #endif
    
  


    if((x>COR_BTN_RECALI_X && x<(COR_BTN_RECALI_X+BTN_RECALI_X)) && (y>COR_BTN_RECALI_Y && y<(COR_BTN_RECALI_Y+BTN_RECALI_Y))){
      // call screen calibration
        touch_calibrate();
    }
    
    
    if((x>COR_BTN_GET_X && x<(COR_BTN_GET_X+BTN_GET_X)) && (y>COR_BTN_GET_Y && y<(COR_BTN_GET_Y+BTN_GET_Y))){
     
       uint32_t t = millis();


    // Fetch the jpg file from the specified URL, examples only, from imgur
    bool loaded_ok = getFile(camServer, "/capture.jpg"); // Note name preceded with "/"

    t = millis() - t;
    if (loaded_ok) { Serial.print(t); Serial.println(" ms to download"); }

    // List files stored in SPIFFS, should have the file now
    listSPIFFS();

    t = millis();

    // Now draw the SPIFFS file
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawFsJpg(110, 50, "/capture.jpg");

    t = millis() - t;
    Serial.print(t); Serial.println(" ms to draw to TFT");
    
    //Delete the file
    SPIFFS.remove("/capture.jpg");
    }

  }
    //delay(1000);
}





