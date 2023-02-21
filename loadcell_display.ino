


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
#include "EEPROM.h"

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
const int UNIT_BUTTON_PIN = 35;

int last_tare_state = 0;
int last_unit_state = 0;

// #define CTS_TO_KGS (22.6796 / 2030032.0)
// #define CTS_TO_LBS (50.0 / 2030032.0)
// #define CTS_TO_OZ (800.0 / 2030032.0)

float CTS_AT_50_LBS = (0.0);

float CTS_TO_KGS = (0.0);
float CTS_TO_LBS = (0.0);
float CTS_TO_OZS = (0.0);

//Modbus Register List
enum MBR{
    READOUT_LSB,
    READOUT_MSB,
    CTS_AT_50_LBS_LSB,
    CTS_AT_50_LBS_MSB,
    READOUT_UNITS,
    RAW_COUNTS_0,
    RAW_COUNTS_1,
    RAW_COUNTS_2,
    RAW_COUNTS_3,

    //Leave this one
    TOTAL_REGS_SIZE
};

enum EEPROM_ADDRESSES {
    CTS_AT_50_LBS_EADDR = 0,
    EEPROM_SIZE = 64
};

//Mode select
enum Mode_Select{
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
    uint8_t b[4];
    uint16_t i[2];
    float f;
} converter;

void update_scaling(float cts_at_50_lbs) {
    CTS_AT_50_LBS = cts_at_50_lbs;
    CTS_TO_KGS = (22.6796 / cts_at_50_lbs);
    CTS_TO_LBS = (50.0 / cts_at_50_lbs);
    CTS_TO_OZS = (800.0 / cts_at_50_lbs);
}

void EEPROM_Write_float(uint8_t address, float value) {
    converter.f = value;
    EEPROM.writeByte(address + 0, converter.b[0]);
    EEPROM.writeByte(address + 1, converter.b[1]);
    EEPROM.writeByte(address + 2, converter.b[2]);
    EEPROM.writeByte(address + 3, converter.b[3]);
    EEPROM.commit();
}

float EEPROM_Read_float(uint8_t address) {
    converter.b[0] = EEPROM.readByte(address + 0);
    converter.b[1] = EEPROM.readByte(address + 1);
    converter.b[2] = EEPROM.readByte(address + 2);
    converter.b[3] = EEPROM.readByte(address + 3);
    return converter.f;
}

void EEPROM_Write_uint16_t(uint8_t address, uint16_t lsb, uint16_t msb) {
    converter.i[0] = lsb;
    converter.i[1] = msb;
    for (int a = 0; a < 4; a++) EEPROM.write(address + a, converter.b[a]);
    EEPROM.commit();
}

uint16_t* EEPROM_Read_uint16_t(uint8_t address) {
    for (int a = 0; a < 4; a++) converter.b[a] = EEPROM.read(address + a);
    return converter.i;
}

void MB_Write_float(uint16_t lsb, float value) {
    if (lsb == CTS_AT_50_LBS_LSB) {
        update_scaling(value);
    }
    converter.f = value;
    MBRTU.Hreg(lsb + 0, converter.i[0]);
    MBRTU.Hreg(lsb + 1, converter.i[1]);
    MBWiFi.Hreg(lsb + 0, converter.i[0]);
    MBWiFi.Hreg(lsb + 1, converter.i[1]);
}

void MB_Write_uint16_t(uint16_t addr, uint16_t value) {
    MBRTU.Hreg(addr, value);
    MBWiFi.Hreg(addr, value);
}

void MB_Write_Long(uint16_t addr, long value) {
    union {
        uint16_t i[4];
        long l;
    } long_converter;
    long_converter.l = value;
    for (int r = 0; r < 4; r++) {
        MBWiFi.Hreg(addr + r, long_converter.i[r]);
        MBRTU.Hreg(addr + r, long_converter.i[r]);
    }
}

float MBRTU_Read_float(uint16_t lsb) {
    converter.i[0] = MBRTU.Hreg(lsb + 0);
    converter.i[1] = MBRTU.Hreg(lsb + 1);
    return converter.f;
}

float MBWiFi_Read_float(uint16_t lsb) {
    converter.i[0] = MBWiFi.Hreg(lsb + 0);
    converter.i[1] = MBWiFi.Hreg(lsb + 1);
    return converter.f;
}

void MB_Poll() {
    MBRTU.task();
    MBWiFi.task();
}

void wait_on_scale() {
    while(!scale.is_ready()) {
        last_tare_state = digitalRead(TARE_BUTTON_PIN);
        last_unit_state = digitalRead(UNIT_BUTTON_PIN);
        MB_Poll();
        if (digitalRead(TARE_BUTTON_PIN) == 0 && last_tare_state == 1) {
            update_scaling(MBRTU_Read_float(CTS_AT_50_LBS_LSB));
            EEPROM_Write_float(CTS_AT_50_LBS_EADDR, CTS_AT_50_LBS);
            scale.tare();
        }
        if (digitalRead(UNIT_BUTTON_PIN) == 0 && last_unit_state == 1) {
            changeMode();
        }
    }
}
int16_t readout_x1, readout_y1, readout_x2, readout_y2;

void setup(void) {
 
    // set up modbus server here
    Serial.begin(115200, SERIAL_8N1);
    MBRTU.begin(&Serial);
    MBRTU.slave(1);
    MBRTU.addHreg(0, 0, TOTAL_REGS_SIZE);

    WiFi.softAP("LOAD_CELL_HOTSPOT", "3rdWaveLabs");
    myIP = WiFi.softAPIP();

    MBWiFi.server(504);
    MBWiFi.addHreg(0, 0, TOTAL_REGS_SIZE);

    while(!EEPROM.begin(EEPROM_SIZE)) {}
    CTS_AT_50_LBS = EEPROM_Read_float(CTS_AT_50_LBS_EADDR);
    MB_Write_float(CTS_AT_50_LBS_LSB, CTS_AT_50_LBS);
    update_scaling(CTS_AT_50_LBS);
    
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.tare();

    tft.begin();

    tft.setRotation(1);

    pinMode(TARE_BUTTON_PIN, INPUT);
    pinMode(UNIT_BUTTON_PIN, INPUT);

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set the font colour AND the background colour
    // so the anti-aliasing works

    tft.setCursor(0, 0);// Set cursor at top left of screen

    tft.loadFont(AA_FONT_LARGE);// Must load the font first

    tft.println("Load Cell:");// println moves cursor down for a new line
    readout_x1 = tft.getCursorX();
    readout_y1 = tft.getCursorY();
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.print("- - - -");
    readout_x2 = tft.getCursorX();
    tft.println();
    readout_y2 = tft.getCursorY();
    tft.loadFont(AA_FONT_SMALL);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
    tft.print("\nMB RTU: ");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("Unit 1, 115200, 8N1");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print("MB WIFI: ");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print(myIP);
    tft.println(":504");
}


void loop() {

    tft.loadFont(AA_FONT_LARGE);// Must load the font first
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set the font colour AND the background colour
    // so the anti-aliasing works

    tft.setCursor(readout_x1, readout_y1);// Set cursor at top left of screen
    tft.fillRect(readout_x1, readout_y1, readout_x2-readout_x1, readout_y2-readout_y1, TFT_BLACK);

    
    readout_x1 = tft.getCursorX();
    readout_y1 = tft.getCursorY();

    if (scale.is_ready()) {
        float scaled_value;
        if (mode == KGS) {
            scaled_value = (value * CTS_TO_KGS);
            tft.print(scaled_value, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.print(" kg");
        }
        else if (mode == LBS) {
            scaled_value = (value * CTS_TO_LBS);
            tft.print(scaled_value, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.print(" lbs");
        }
        else if (mode == OZS) {
            scaled_value = (value * CTS_TO_OZS);
            tft.print(value * CTS_TO_OZS, 3);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.print(" oz");
        }
        else if (mode == CTS) {
            tft.loadFont(AA_FONT_SMALL);
            scaled_value = (value);
            tft.print(scaled_value);
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.print(" cts");
        }
        MB_Write_float(READOUT_LSB, scaled_value);
        MB_Write_uint16_t(READOUT_UNITS, mode);
        MB_Write_Long(RAW_COUNTS_0, scale.raw_value);
    } 
    else {
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.print("- - - -");
    }

    
    readout_x2 = tft.getCursorX();
    tft.println();
    readout_y2 = tft.getCursorY();

    value = scale.get_value(1);
    wait_on_scale();
}
