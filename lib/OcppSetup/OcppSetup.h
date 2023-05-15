#ifndef OCPPSETUPFILE_H
#define OCPPSETUPFILE_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "FS.h"
#include "SPIFFS.h"
#include "time.h"
#include <Ticker.h>
// MFRC522 library
#include <SPI.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <FS.h>

#define RED 0xff0000;
#define GREEN 0x00ff00;
#define BLUE 0x0000ff;

#define Amperage_Pin 4 // modulated as PWM

#define EV_Plug_Pin 36 // Input pin | Read if an EV is connected to the EVSE
#define EV_Plugged HIGH
#define EV_Unplugged LOW

#define OCPP_Charge_Permission_Pin 10 // Output pin | Signal if OCPP allows / forbids energy flow
#define OCPP_Charge_Permitted HIGH
#define OCPP_Charge_Forbidden LOW

#define EV_Charge_Pin 35 // Input pin | Read if EV requests energy (corresponds to SAE J1772 State C)
#define EV_Charging LOW
#define EC_Suspended HIGH

#define OCPP_Availability_Pin 9 // Output pin | Signal if this EVSE is out of order (set by Central System)
#define OCPP_Available HIGH
#define OCPP_Unavailable LOW

#define EVSE_Ground_Fault_Pin 34 // Input pin | Read ground fault detector
#define EVSE_Grud_Faulted HIGH
#define EVSE_Ground_Clear LOW

#define EV_Plug_Pin 36 // Input pin | Read if an EV is connected to the EVSE
#define EV_Plugged HIGH
#define EV_Unplugged LOW

#define FORMAT_SPIFFS_IF_FAILED true
#define BUZZER_PIN 2


#define MYPORT_TX 21
#define MYPORT_RX 22


#define WIFI_MEM_H 0x11
#define WIFI_MEM_L 0x00

#define RFID_MEM_H 0x10
#define RFID_MEM_L 0x00

#define SERVER_MEM_H 0x12
#define SERVER_MEM_L 0x00

#define STATE_MEM_H 0x13
#define STATE_MEM_L 0x00

#define QR_MEM_H 0x14
#define QR_MEM_L 0x00

#define MAIN_MEM_H 0x15
#define MAIN_MEM_L 0x00

#define WH_MEM_H 0x16
#define WH_MEM_L 0X00

#define V_MEM_H 0x17
#define V_MEM_L 0x00

#define I_MEM_H 0x18
#define I_MEM_L 0x00

#define TIME_MEM_H 0x19
#define TIME_MEM_L 0x00


#define UNAUTHENTICATED_MAIN 1
#define AUTHENTICATED_MAIN 0
#define BLANK_MAIN 2
#define THANKYOU_MAIN 3
#define SOS_MAIN 4
#define MOREINFO_MAIN 5


#define BLANK_GREEN 0
#define AVL_GREEN 1
#define CHAR_GREEN 2
#define PRE_GREEN 3


// extern bool connection_EV;
// extern bool charge_EV;

class OcppSetup
{

private:
    void _touchWrite(int value, byte address_h, byte address_l);
    
    

public:
    bool server_disconnected_file_management;

    bool connected_to_wifi, to_count, disconnected;
    bool wifi_disconnected = true;
    int count;

    long gmtOffset_sec;
    int daylightOffset_sec;
    unsigned long time_now;
    int period;
    char timeSeconds[10];
    char timeMin[3];
    char timeHour[10];
    char timeDay[10];
    char timeMonth[10];
    char timeYear[5];

    const char *dirname;
    const char *path;
    char *status_copy;
    bool initialized_lcd = false;
    bool initialized_file = false;
    bool initialized_led = false;

    struct tm timeinfo;
    Ticker periodicTicker;
    Ticker onceTicker;

    int screen;

    byte read_signal;
    byte tdata[200];
    int count_for_data;
    int len_of_data;
    bool start_count;
    byte address[2];
    // int S=0,HH=0,MM=0;
    void ledChangeColour(int R, int G, int B);

    void lcdInitialize();
    void lcdPrint(const char *msg, int x, int y);
    void lcdPrint(const char *msg);
    void lcdClear();
    void lcdPrintint(int msg, int x, int y);

    void touchSetup();
    byte * touchRead();
    void meterWrite(double WH, double V, double I);
    void dwin_server_wifi(int strength_server);
    void dwin_rfid(bool auth);
    void dwin_state(int value);
    void dwin_main(int value);

    void fileManageInitialize(const char *ssid, const char *pass);
    void ServerDisconnect();
    void ServerConnect();

    void setStatus(char *val);
    char *getStatus();

    void setMeterValue(char *val, char *type);
    char *getMeterValue();
    char *getMeterAnal();

    // void setMeterStopValue(char *val, char* type);
    // char *getMeterStopValue();
    // char *getMeterStopAnal();

    void setServerStatus(char *serverstatus);
    char *getServerStatus();

    void sendCSVFile(char auth[]);
    void sendCSVFile(const char *server_name, char input[]);
    // void chargetime();
    void buzz();
    void buzzcontinue();
    void buzzstop();

    
};

extern OcppSetup ocppsetup;
#endif