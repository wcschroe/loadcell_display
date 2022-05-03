/*
This sketch is the same as the Font_Demo_1 example, except the fonts in this
example are in a FLASH (program memory) array. This means that processors
such as the STM32 series that are not supported by a SPIFFS library can use
smooth (anti-aliased) fonts.
*/

/*
There are four different methods of plotting anti-aliased fonts to the screen.

This sketch uses method 1, using tft.print() and tft.println() calls.

In some cases the sketch shows what can go wrong too, so read the comments!

The font is rendered WITHOUT a background, but a background colour needs to be
set so the anti-aliasing of the character is performed correctly. This is because
characters are drawn one by one.

This method is good for static text that does not change often because changing
values may flicker. The text appears at the tft cursor coordinates.

It is also possible to "print" text directly into a created sprite, for example using
spr.println("Hello"); and then push the sprite to the screen. That method is not
demonstrated in this sketch.

*/

//  A processing sketch to create new fonts can be found in the Tools folder of TFT_eSPI
//  https://github.com/Bodmer/TFT_eSPI/tree/master/Tools/Create_Smooth_Font/Create_font

//  This sketch uses font files created from the Noto family of fonts:
//  https://www.google.com/get/noto/

#include "NotoSansBold15.h"
#include "NotoSansBold36.h"

// The font names are arrays references, thus must NOT be in quotes ""
#define AA_FONT_SMALL NotoSansBold15
#define AA_FONT_LARGE NotoSansBold36

#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <HX711.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <ModbusIP_ESP8266.h>

// Modbus Registers Offsets
const int TEST_HREG = 100;

//ModbusIP object
ModbusIP mb;

//IP Object
IPAddress myIP;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 22;
const int LOADCELL_SCK_PIN = 21;

#define CTS_TO_KGS (0.1880 / 16700.0)
#define CTS_TO_LBS (0.4125 / 16700.0)
#define CTS_TO_OZ (6.6 / 16700.0)

enum {
    KGS,
    LBS,
    OZ,
    CTS,
};

int mode = KGS;

HX711 scale;

TFT_eSPI tft = TFT_eSPI();

double value = 0;

void changeMode() {
    mode++;
    if (mode > CTS) {
        mode = KGS;
    }
}

union {
    uint16_t i[2];
    float f;
} converter;

void wait(unsigned long ms) {
    unsigned long start = millis();
    unsigned long now = millis();
    bool tare_sent = false;
    bool change_sent = false;
    while((now - start) < ms) {
        now = millis();
        mb.task();
        if (digitalRead(0) == 0 && !tare_sent) {
            scale.tare();
            tare_sent = true;
        }
        if (digitalRead(35) == 0 && !change_sent) {
            changeMode();
            change_sent = true;
        }
    }
}

void setup(void) {
 
    WiFi.softAP("LOAD_CELL_HOTSPOT", "3rdWaveLabs");
    myIP = WiFi.softAPIP();

    mb.server(504);
    mb.addHreg(0, 0, 100);

    tft.begin();

    tft.setRotation(1);

    pinMode(0, INPUT);
    pinMode(35, INPUT);

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.tare();
}


void loop() {
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set the font colour AND the background colour
    // so the anti-aliasing works

    tft.setCursor(0, 0);// Set cursor at top left of screen

    tft.loadFont(AA_FONT_LARGE);// Must load the font first

    tft.println("Load Cell:\n");// println moves cursor down for a new line

    if (scale.is_ready()) {
        if (value < 0)
            value = value * -1;
        if (mode == KGS) {
            double value_in_kg = (value * CTS_TO_KGS);
            if (value_in_kg < 1) {
                tft.print(value_in_kg * 1000, 1);
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.println(" g");
            }
            else {
                tft.print(value_in_kg, 3);
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.println(" kg");
            }
            converter.f = value_in_kg;
            mb.Hreg(0, converter.i[0]);
            mb.Hreg(1, converter.i[1]);
        }
        else if (mode == LBS) {
            tft.print(value * CTS_TO_LBS, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.println(" lbs");
        }
        else if (mode == OZ) {
            tft.print(value * CTS_TO_OZ, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.println(" oz");
        }
        else if (mode == CTS) {
            tft.print(value);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.println(" cts");
        }
    } 
    else {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.println("- - - -");
    }
    tft.loadFont(AA_FONT_SMALL);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft.print("MB address: ");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print(myIP);
    tft.println(":504");

    mb.task();
    value = scale.get_value(5);
    wait(250);
}
