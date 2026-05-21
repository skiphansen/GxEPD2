// GxEPD2_HelloWorld.ino by Jean-Marc Zingg
//
// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Display Library based on Demo Example from Good Display: https://www.good-display.com/companyfile/32/
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

// Supporting Arduino Forum Topics (closed, read only):
// Good Display ePaper for Arduino: https://forum.arduino.cc/t/good-display-epaper-for-arduino/419657
// Waveshare e-paper displays with SPI: https://forum.arduino.cc/t/waveshare-e-paper-displays-with-spi/467865
//
// Add new topics in https://forum.arduino.cc/c/using-arduino/displays/23 for new questions and issues

// see GxEPD2_wiring_examples.h for wiring suggestions and examples
// if you use a different wiring, you need to adapt the constructor parameters!

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
//#include <GFX.h>

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "logging.h"

// select the display class and display driver class in the following file (new style):
// #include "GxEPD2_display_selection_new_style.h"

// or select the display constructor line in one of the following files (old style):
// #include "GxEPD2_display_selection.h"
// #include "GxEPD2_display_selection_added.h"

// alternately you can copy the constructor from GxEPD2_display_selection.h or GxEPD2_display_selection_added.h to here
// e.g. for Wemos D1 mini:
//GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 2, /*BUSY=D2*/ 4)); // GDEH0154D67
// adapt the constructor parameters to your wiring
// GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> display(GxEPD2_DRIVER_CLASS(/*CS=*/ EPD_CS, /*DC=*/ 8, /*RST=*/ 9, /*BUSY=*/ 7));
#define MAX_DISPLAY_BUFFER_SIZE 65536ul
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
GxEPD2_4C<GxEPD2_740c_E2741QS0B3, MAX_HEIGHT(GxEPD2_740c_E2741QS0B3)> display(GxEPD2_740c_E2741QS0B3(/*CS=*/ 44, /*DC=*/ 10, /*RST=*/ 38, /*BUSY=*/ 4));

// SPIClass hspi(HSPI);


// for handling alternative SPI pins (ESP32, RP2040) see example GxEPD2_Example.ino

void helloWorld();
void setup()
{

   Serial.begin(115200);
   while(!Serial);
   delay(250);
   LOG("Hello world!\n");
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02

   // remap spi
   SPI.end();
   SPI.begin(/* sck */ D8, /* miso */ -1,/* mosi */ D10, /* ss */ -1); // remap hspi for EPD (swap pins)
   display.epd2.selectSPI(SPI, SPISettings(5000000, MSBFIRST, SPI_MODE0));

  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  LOG("Press a key to continue");
  while(!Serial.available());
  int incomingByte = Serial.read();
  LOG("\n");
  helloWorld();
  display.hibernate();

}

const char HelloWorld[] = "Hello World!";

void helloWorld()
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloWorld);
    display.setCursor(x, y + (2 * tbh));
    display.setTextColor(GxEPD_RED);
    display.print("Hello Red");
    display.setCursor(x, y + (4 * tbh));
    display.setTextColor(GxEPD_YELLOW);
    display.print("Hello Yellow");
    display.setCursor(0,tbh);
    display.setTextColor(GxEPD_BLACK);
    display.print("Hello 0,0");
  }
  while (display.nextPage());
}

void loop() {};
