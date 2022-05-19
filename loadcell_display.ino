


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
#include <ModbusRTU.h>

// Modbus Registers Offsets
const int TEST_HREG = 100;

//TODO: Modbus object


//Modbus objects
ModbusRTU MBRTU;
ModbusIP MBWiFi;

//IP Object
IPAddress myIP;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 22;
const int LOADCELL_SCK_PIN = 21;
const int TARE_BUTTON_PIN = 0;
const int UNIT_BUTTON_PIN = 25;

// #define CTS_TO_KGS (0.1880 / 16700.0)
// #define CTS_TO_LBS (0.4125 / 16700.0)
// #define CTS_TO_OZ (6.6 / 16700.0)

#define CTS_TO_KGS (22.6796 / 2030032.0)
#define CTS_TO_LBS (50.0 / 2030032.0)
#define CTS_TO_OZ (800.0 / 2030032.0)

//2.4700598802395209580838323353293e-5
//2.4630153613342055691732938199989e-5

enum {
    KGS,
    LBS,
    OZS,
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

void wait_on_scale() {
    bool tare_sent = false;
    bool change_sent = false;
    while(!scale.is_ready()) {
        MBRTU.task();
        MBWiFi.task();
        if (digitalRead(TARE_BUTTON_PIN) == 0 && !tare_sent) {
            scale.tare();
            tare_sent = true;
        }
        if (digitalRead(UNIT_BUTTON_PIN) == 0 && !change_sent) {
            changeMode();
            change_sent = true;
        }
    }
}

void setup(void) {
 
    // set up modbus server here
    Serial.begin(115200, SERIAL_8N1);
    MBRTU.begin(&Serial);
    MBRTU.slave(1);
    MBRTU.addHreg(0, 0, 100);

    WiFi.softAP("LOAD_CELL_HOTSPOT", "3rdWaveLabs");
    myIP = WiFi.softAPIP();

    MBWiFi.server(504);
    MBWiFi.addHreg(0, 0, 100);

    tft.begin();

    tft.setRotation(1);

    pinMode(TARE_BUTTON_PIN, INPUT);
    pinMode(UNIT_BUTTON_PIN, INPUT);

    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.tare();
}


void loop() {
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set the font colour AND the background colour
    // so the anti-aliasing works

    tft.setCursor(0, 0);// Set cursor at top left of screen

    tft.loadFont(AA_FONT_LARGE);// Must load the font first

    tft.println("Load Cell:");// println moves cursor down for a new line

    if (scale.is_ready()) {
        if (value < 0)
            value = value * -1;
        if (mode == KGS) {
            double value_in_kg = (value * CTS_TO_KGS);
            
            tft.print(value_in_kg, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.println(" kg");

            converter.f = value_in_kg;
            MBRTU.Hreg(0, converter.i[0]);
            MBRTU.Hreg(1, converter.i[1]);
            MBWiFi.Hreg(0, converter.i[0]);
            MBWiFi.Hreg(1, converter.i[1]);
        }
        else if (mode == LBS) {
            tft.print(value * CTS_TO_LBS, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.println(" lbs");
        }
        else if (mode == OZS) {
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
    
    tft.print("\nMB RTU: ");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("Addr 1, 115200, 8N1");
    tft.print("MB WIFI: ");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print(myIP);
    tft.println(":504");

    MBRTU.task();
    MBWiFi.task();
    value = scale.get_value(5);
    wait_on_scale();
}
