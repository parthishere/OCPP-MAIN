#include "OcppSetup.h"
#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <base64.h>

#ifndef LCD_INITIALIZED_IN_FILE
#define LCD_INITIALIZED_IN_FILE

void checkStatus();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void readFile(fs::FS &fs, const char *path);
void deleteFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void testFileIO(fs::FS &fs, const char *path);

namespace Parth
{
    LiquidCrystal_I2C lcd(0x27, 20, 4);
    EspSoftwareSerial::UART myPort;
    int RledChannel = 1;
    int GledChannel = 2;
    int BledChannel = 3;
    int resolution = 8;
};
using namespace Parth;

bool initialized_lcd = false;
bool initialized_led = false;
// bool connection_EV = false;
// bool charge_EV = false;
char *ssid;
char *pass;

bool connected_to_wifi, to_count, disconnected, wifi_disconnected;
bool server_disconnected_file_management = false;
int count;

char *ntpServer = "pool.ntp.org";
long gmtOffset_sec = 0;
int daylightOffset_sec = 3600;
unsigned long time_now = 0;
int period = 1000;

char timeSeconds[10];
char timeMin[3];
char timeHour[10];
char timeDay[10];
char timeMonth[10];
char timeYear[5];

byte read_signal;
byte tdata[200];
int count_for_data;
int len_of_data;
bool start_count;
byte address[2];

static char *status = "null";
static char *meter_value = "null";
static char *meter_type = "W";
static char *server_status = "ERR";

char *status_copy;

Ticker periodicTicker;
Ticker onceTicker;

void OcppSetup::lcdInitialize()
{

    Serial.println("LCD Initialized !");
    if (initialized_lcd == false)
    {
        initialized_lcd = true;
        lcd.init();
        lcd.clear();
        lcd.backlight();
    }
    else
    {
        Serial.println("LCD Initialized !");
    }
}

void OcppSetup::ledChangeColour(int R, int G, int B)
{

    ledcWrite(RledChannel, R);
    ledcWrite(BledChannel, B);
    ledcWrite(GledChannel, G);
    // HH = 0;
    // MM = 0;
    // S = 0;
}
// void OcppSetup::chargetime()
// {
//     S++;
//     delay(1000);
//     if (S == 59)
//     {
//         MM++;
//         S = 0;
//     }
//     if(MM == 59)
//     {
//         HH++;
//         MM = 0;
//     }
//     Serial.printf("\n %d : %d : %d",HH,MM,S);
// }

void OcppSetup::lcdPrint(const char *msg, int x, int y)
{
    lcd.setCursor(y, x);
    lcd.print(msg);
}

void OcppSetup::lcdPrint(const char *msg)
{
    lcd.setCursor(0, 0);
    lcd.print(msg);
}
void OcppSetup::lcdPrintint(int msg, int x, int y)
{
    lcd.setCursor(y, x);
    lcd.print(msg);
}
void OcppSetup::lcdClear()
{
    lcd.clear();
}

void OcppSetup::touchSetup(){
    myPort.begin(115200, SWSERIAL_8N1, MYPORT_RX, MYPORT_TX, false);
    if (!myPort) {  // If the object did not initialize, then its configuration is invalid
        Serial.println("Invalid EspSoftwareSerial pin configuration, check config");
        while (1) {  // Don't continue with invalid configuration
        delay(1000);
        }
    }
}

byte * OcppSetup::touchRead(){
    while (myPort.available()) {
        read_signal = myPort.read();
        // Serial.print(read_signal, HEX);
        // Serial.print(" ");
        if (read_signal == 0x5a) {
        start_count = true;
        }
        if (start_count == true) {
        count_for_data++;
        }

        if (count == 3) {
        len_of_data = read_signal;  //05
        }

        else if (count > 4) {
        tdata[len_of_data - 3];
        for (int i = 0; i < len_of_data - 2; i++) {

            if (i == 0) {
            
            while (!myPort.available())
                ;
            address[1] = myPort.read();
            } else if (i == 1) {
            while (!myPort.available())
                ;
            address[0] = myPort.read();
            }  else if(i>1) {
            while (!myPort.available())
                ;
            tdata[i - 2] = myPort.read();
            }
        }
        start_count = false;
        count = 0;

        Serial.print("Address: ");
        for (int i = 0; i < 2; i++) {
            Serial.print(address[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        Serial.print("Data: ");
        for (int i = 0; i < len_of_data - 3 - 1; i++) {
            Serial.print(tdata[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        Serial.println();
        
        }
    }

    return address, tdata;

} 

void OcppSetup::_touchWrite(int value, byte address_h, byte address_l){
    byte array[8] = { 0x5a, 0xa5, 0x05, 0x82, address_h, address_l, 0x00};
}

void OcppSetup::meterWrite(double WH, double V, double I){
    dwin_state(CHAR_GREEN);
    
    byte wh_write[8];
    byte * wh = reinterpret_cast<byte*> (&WH);
    memcpy(wh_write, wh, 8);
    byte array_wh[14] = { 0x5a, 0xa5, 0x0b, 0x82, WH_MEM_H, WH_MEM_L, wh_write[7], wh_write[6], wh_write[5], wh_write[4], wh_write[3], wh_write[2], wh_write[1], wh_write[0]};

    myPort.write(array_wh, 14);


    byte v_write[8];
    byte * v = reinterpret_cast<byte*> (&V);
    memcpy(v_write, v, 8);
    byte array_v[14] = { 0x5a, 0xa5, 0x0b, 0x82, V_MEM_H, V_MEM_L, v_write[7], v_write[6], v_write[5], v_write[4], v_write[3], v_write[2], v_write[1], v_write[0]};

    myPort.write(array_v, 14);


    byte i_write[8];
    byte * i = reinterpret_cast<byte*> (&I);
    memcpy(i_write, i, 8);
    byte array_i[14] = { 0x5a, 0xa5, 0x0b, 0x82, I_MEM_H, I_MEM_L, i_write[7], i_write[6], i_write[5], i_write[4], i_write[3], i_write[2], i_write[1], i_write[0]};

    myPort.write(array_i, 14);
    

}



void OcppSetup::dwin_server_wifi(int strength_server){
    _touchWrite(strength_server, SERVER_MEM_H, SERVER_MEM_L);
    int wifi = map(WiFi.RSSI(), 0,50, 0, 5);
    _touchWrite(wifi, WIFI_MEM_H, WIFI_MEM_L);
}

void OcppSetup::dwin_main(int value){
    
    switch(value){
        case UNAUTHENTICATED_MAIN:
            _touchWrite(UNAUTHENTICATED_MAIN, MAIN_MEM_H, MAIN_MEM_L);
            break;
         case AUTHENTICATED_MAIN:
            _touchWrite(AUTHENTICATED_MAIN, MAIN_MEM_H, MAIN_MEM_L);
            break;
         case BLANK_MAIN:
            _touchWrite(BLANK_MAIN, MAIN_MEM_H, MAIN_MEM_L);
            break;
         case THANKYOU_MAIN:
            _touchWrite(THANKYOU_MAIN, MAIN_MEM_H, MAIN_MEM_L);
            break;
        case SOS_MAIN:
            _touchWrite(SOS_MAIN, MAIN_MEM_H, MAIN_MEM_L);
            break;
        case MOREINFO_MAIN:
            _touchWrite(MOREINFO_MAIN, MAIN_MEM_H, MAIN_MEM_L);
            break;

    }
}

void OcppSetup::dwin_rfid(bool auth){
    int hey = auth == true? 1 : 2;
    _touchWrite(hey, RFID_MEM_H, RFID_MEM_L);
    int main = auth == true ? AUTHENTICATED_MAIN: UNAUTHENTICATED_MAIN;
    dwin_main(main);
}

 void OcppSetup::dwin_state(int value){
    switch(value){
        case BLANK_GREEN:
            _touchWrite(BLANK_GREEN, STATE_MEM_H, STATE_MEM_L);
            break;
         case AVL_GREEN:
            _touchWrite(AVL_GREEN, STATE_MEM_H, STATE_MEM_L);
            break;
         case CHAR_GREEN:
            _touchWrite(CHAR_GREEN, STATE_MEM_H, STATE_MEM_L);
            break;
         case PRE_GREEN:
            _touchWrite(PRE_GREEN, STATE_MEM_H, STATE_MEM_L);
            break;
    }
 }



void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
// void OcppSetup::chargetime()
// {
        // S++;  
        // delay ( 1000 );  
        // if ( S > 59 )  
        // {  
        //     MM++;  
        //     S = 0;  
        // }  
        // if ( MM > 59 )  
        // {  
        //     HH++;  
        //     MM = 0;  
        // }  
        // Serial.printf("\n %d : %d : %d",HH,MM,S);
        // ocppsetup.lcdPrintint(HH, 2,7);
        // ocppsetup.lcdPrint(":", 2,9);
        // ocppsetup.lcdPrintint(MM, 2,10);
        // ocppsetup.lcdPrint(":", 2,12);
        // ocppsetup.lcdPrintint(S,2,13);
// }
void readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing");
    uint32_t start = millis();
    for (i = 0; i < 2048; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("- failed to open file for reading");
    }
}

void OcppSetup::fileManageInitialize(const char *ssid, const char *pass)
{
    if (initialized_file)
    {
    }
    else
    {

        connected_to_wifi = false;
        to_count = false;
        disconnected = true;
        count = 0;

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }

        listDir(SPIFFS, "/", 0);

        // periodicTicker.attach_ms(5000, checkStatus);

        writeFile(SPIFFS, "/hello.csv", "TYPE,DAY,MONTH,YEAR,HOURS,MINUTES,SECONDS,CODE\n");

        readFile(SPIFFS, "/hello.csv");

        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        ssid = ssid;
        pass = pass;
    }
}

void OcppSetup::ServerDisconnect()
{

    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
    if (WiFi.status() == WL_CONNECTED)
    {
        wifi_disconnected = false;
        if (!getLocalTime(&timeinfo))
        {
            Serial.println("Failed to obtain time in ocpp socket file");
            return;
        }
        Serial.println("This is server disconnection info");

        strftime(timeSeconds, 10, "%S", &timeinfo);
        strftime(timeMin, 3, "%M", &timeinfo);
        strftime(timeHour, 10, "%H", &timeinfo);
        strftime(timeDay, 10, "%A", &timeinfo);
        strftime(timeMonth, 10, "%B", &timeinfo);
        strftime(timeYear, 5, "%Y", &timeinfo);
        Serial.println();
        if (!server_disconnected_file_management)
        {
            // First time connection after disconnection

            appendFile(SPIFFS, "/hello.csv", "DISCONNECTION_SERVER,");
            appendFile(SPIFFS, "/hello.csv", timeDay);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeMonth);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeYear);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeHour);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeMin);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeSeconds);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", "500");
            appendFile(SPIFFS, "/hello.csv", "\n");
        }
        // File file = SPIFFS.open(pathWithExtension, "r");
        // size_t sent = server.streamFile(file, "text/css");
        // file.close();
    }
    else
    {
        wifi_disconnected = true;
        ocppsetup.buzz();
        // server disconnect
        if (!server_disconnected_file_management)
        {
            // First time connection after disconnection

            appendFile(SPIFFS, "/hello.csv", "DISCONNECTION_WIFI,");
            appendFile(SPIFFS, "/hello.csv", timeDay);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeMonth);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeYear);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeHour);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeMin);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeSeconds);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", "500");
            appendFile(SPIFFS, "/hello.csv", "\n");
        }
    }
    readFile(SPIFFS, "/hello.csv");
    server_disconnected_file_management = true;
}

// void OcppSetup::prev_disconnect()
// {
// }

void OcppSetup::ServerConnect()
{
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time in ocpp socket file");
    }
    Serial.println("This is server Connection info");

    strftime(timeSeconds, 10, "%S", &timeinfo);
    strftime(timeMin, 3, "%M", &timeinfo);
    strftime(timeHour, 10, "%H", &timeinfo);
    strftime(timeDay, 10, "%A", &timeinfo);
    strftime(timeMonth, 10, "%B", &timeinfo);
    strftime(timeYear, 5, "%Y", &timeinfo);
    Serial.println();
    if (server_disconnected_file_management)
    {
        // First time connection after disconnection
        ocppsetup.buzz();
        if (WiFi.status() == WL_CONNECTED && wifi_disconnected)
        {
            wifi_disconnected = false;
            appendFile(SPIFFS, "/hello.csv", "CONNECTION_WIFI,");
            appendFile(SPIFFS, "/hello.csv", timeDay);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeMonth);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeYear);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeHour);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeMin);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", timeSeconds);
            appendFile(SPIFFS, "/hello.csv", ",");

            appendFile(SPIFFS, "/hello.csv", "100");
            appendFile(SPIFFS, "/hello.csv", "\n");
        }

        appendFile(SPIFFS, "/hello.csv", "CONNECTION_SERVER,");
        appendFile(SPIFFS, "/hello.csv", timeDay);
        appendFile(SPIFFS, "/hello.csv", ",");

        appendFile(SPIFFS, "/hello.csv", timeMonth);
        appendFile(SPIFFS, "/hello.csv", ",");

        appendFile(SPIFFS, "/hello.csv", timeYear);
        appendFile(SPIFFS, "/hello.csv", ",");

        appendFile(SPIFFS, "/hello.csv", timeHour);
        appendFile(SPIFFS, "/hello.csv", ",");

        appendFile(SPIFFS, "/hello.csv", timeMin);
        appendFile(SPIFFS, "/hello.csv", ",");

        appendFile(SPIFFS, "/hello.csv", timeSeconds);
        appendFile(SPIFFS, "/hello.csv", ",");

        appendFile(SPIFFS, "/hello.csv", "100");
        appendFile(SPIFFS, "/hello.csv", "\n");
    }
    server_disconnected_file_management = false;
    readFile(SPIFFS, "/hello.csv");
    // String myFile = "/accepte.html";
    // File file = SPIFFS.open(myFile, "r");
    // size_t sent = server.streamFile(file, "text/csv");
    // server.send(200, "text/html", file);
    // file.close();
}

void OcppSetup::setStatus(char *val)
{
    status = val;
}

char *OcppSetup::getStatus()
{
    status_copy = status;
    return status_copy;
}

void OcppSetup::setMeterValue(char *val, char *type)
{
    meter_value = val;
    meter_type = type;
}

char *OcppSetup::getMeterValue()
{
    return meter_value;
}

char *OcppSetup::getMeterAnal()
{
    return meter_type;
}

void OcppSetup::setServerStatus(char *serverstatus)
{
    server_status = serverstatus;
}

char *OcppSetup::getServerStatus()
{
    return server_status;
}

void OcppSetup::buzz()
{
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
}
void OcppSetup::buzzcontinue()
{
    digitalWrite(BUZZER_PIN, HIGH);
}
void OcppSetup::buzzstop()
{
    digitalWrite(BUZZER_PIN, LOW);
}
void OcppSetup::sendCSVFile(char auth[])
{
    WiFiClientSecure client;
    const char *rootCACertificate =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIFYDCCBEigAwIBAgIQQAF3ITfU6UK47naqPGQKtzANBgkqhkiG9w0BAQsFADA/\n"
        "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
        "DkRTVCBSb290IENBIFgzMB4XDTIxMDEyMDE5MTQwM1oXDTI0MDkzMDE4MTQwM1ow\n"
        "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
        "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwggIiMA0GCSqGSIb3DQEB\n"
        "AQUAA4ICDwAwggIKAoICAQCt6CRz9BQ385ueK1coHIe+3LffOJCMbjzmV6B493XC\n"
        "ov71am72AE8o295ohmxEk7axY/0UEmu/H9LqMZshftEzPLpI9d1537O4/xLxIZpL\n"
        "wYqGcWlKZmZsj348cL+tKSIG8+TA5oCu4kuPt5l+lAOf00eXfJlII1PoOK5PCm+D\n"
        "LtFJV4yAdLbaL9A4jXsDcCEbdfIwPPqPrt3aY6vrFk/CjhFLfs8L6P+1dy70sntK\n"
        "4EwSJQxwjQMpoOFTJOwT2e4ZvxCzSow/iaNhUd6shweU9GNx7C7ib1uYgeGJXDR5\n"
        "bHbvO5BieebbpJovJsXQEOEO3tkQjhb7t/eo98flAgeYjzYIlefiN5YNNnWe+w5y\n"
        "sR2bvAP5SQXYgd0FtCrWQemsAXaVCg/Y39W9Eh81LygXbNKYwagJZHduRze6zqxZ\n"
        "Xmidf3LWicUGQSk+WT7dJvUkyRGnWqNMQB9GoZm1pzpRboY7nn1ypxIFeFntPlF4\n"
        "FQsDj43QLwWyPntKHEtzBRL8xurgUBN8Q5N0s8p0544fAQjQMNRbcTa0B7rBMDBc\n"
        "SLeCO5imfWCKoqMpgsy6vYMEG6KDA0Gh1gXxG8K28Kh8hjtGqEgqiNx2mna/H2ql\n"
        "PRmP6zjzZN7IKw0KKP/32+IVQtQi0Cdd4Xn+GOdwiK1O5tmLOsbdJ1Fu/7xk9TND\n"
        "TwIDAQABo4IBRjCCAUIwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYw\n"
        "SwYIKwYBBQUHAQEEPzA9MDsGCCsGAQUFBzAChi9odHRwOi8vYXBwcy5pZGVudHJ1\n"
        "c3QuY29tL3Jvb3RzL2RzdHJvb3RjYXgzLnA3YzAfBgNVHSMEGDAWgBTEp7Gkeyxx\n"
        "+tvhS5B1/8QVYIWJEDBUBgNVHSAETTBLMAgGBmeBDAECATA/BgsrBgEEAYLfEwEB\n"
        "ATAwMC4GCCsGAQUFBwIBFiJodHRwOi8vY3BzLnJvb3QteDEubGV0c2VuY3J5cHQu\n"
        "b3JnMDwGA1UdHwQ1MDMwMaAvoC2GK2h0dHA6Ly9jcmwuaWRlbnRydXN0LmNvbS9E\n"
        "U1RST09UQ0FYM0NSTC5jcmwwHQYDVR0OBBYEFHm0WeZ7tuXkAXOACIjIGlj26Ztu\n"
        "MA0GCSqGSIb3DQEBCwUAA4IBAQAKcwBslm7/DlLQrt2M51oGrS+o44+/yQoDFVDC\n"
        "5WxCu2+b9LRPwkSICHXM6webFGJueN7sJ7o5XPWioW5WlHAQU7G75K/QosMrAdSW\n"
        "9MUgNTP52GE24HGNtLi1qoJFlcDyqSMo59ahy2cI2qBDLKobkx/J3vWraV0T9VuG\n"
        "WCLKTVXkcGdtwlfFRjlBz4pYg1htmf5X6DYO8A4jqv2Il9DjXA6USbW1FzXSLr9O\n"
        "he8Y4IWS6wY7bCkjCWDcRQJMEhg76fsO3txE+FiYruq9RUWhiF1myv4Q6W+CyBFC\n"
        "Dfvp7OOGAN6dEOM4+qR9sdjoSYKEBpsr6GtPAQw4dy753ec5\n"
        "-----END CERTIFICATE-----\n"
        "";

    client.setCACert(rootCACertificate);
    Serial.println("here 1");
    if (client.connect("fileuploads.up.railway.app", 443))
    {
        Serial.println("here 2");
        File file = SPIFFS.open("/hello.csv", "r");
        String start_request = "";
        String end_request = "";

        String inputString = String(auth);
        String encoded = base64::encode(inputString);
        String auth = "Basic " + encoded;

        start_request = start_request + "\n--AaB03x\n" +
                        "Content-Disposition: form-data; name=\"file\"; filename=\"hello.csv\"\r\n" +
                        "Content-Type: text/csv\r\n\r\n";
        end_request = end_request + "\n--AaB03x--\n";

        uint16_t full_length = start_request.length() + file.size() + end_request.length();
        client.println("POST / HTTP/1.1");
        client.println("Host: fileuploads.up.railway.app");
        client.println("User-Agent: ESP32");
        client.println("Authorization: " + auth);
        client.println("Content-Type: multipart/form-data; boundary=AaB03x");
        client.println("Content-Length: " + String(full_length));
        client.println();
        client.print(start_request);

        while (file.available())
        {
            Serial.print(".");
            client.write(file.read());
        }
        client.println(end_request);

        while (client.available() == 0)
            ;
        while (client.available())
        {
            char c = client.read();
            Serial.write(c);
        }

        Serial.println("");
        client.stop();
        file.close();
        Serial.println("File sent!");
        Serial.println(">>><<<");
        return;
    }
    else
    {
        Serial.println("connection failed");
        return;
    }
}
// void OcppSetup::sendCSVFile(const char *server_name, char input[])
// {
//     WiFiClient client;
//     // const char *server_name = "192.168.1.168"; // Replace with your server's IP address
//     // const int server_port = 8000;

//     if (client.connect(server_name))
//     {
//         // client.setAuthorization("user", "pass");
//         File file = SPIFFS.open("/hello.csv", "r");
//         String start_request = "";
//         String end_request = "";
//         start_request = start_request +
//                         "--AaB03x\r\n" +
//                         "Content-Disposition: form-data; name=\"file\"; filename=\"hello.csv\"\r\n" +
//                         "Content-Type: text/csv\r\n\r\n";
//         end_request = end_request + "\r\n--AaB03x--\r\n";
//         uint16_t full_length = start_request.length() + file.size() + end_request.length();
//         client.println("POST / HTTP/1.1");
//         client.println("Host: " + String(server_name));

//         client.println("User-Agent: ESP32");
//         // char input[] = input;
//         int inputLength = strlen(input);
//         int encodedLength = base64_enc_len(inputLength);
//         char encoded[encodedLength];
//         base64_encode(encoded, input, inputLength);
//         String auth = "Basic " + String(encoded);
//         // Use the 'auth' variable in your code
//         client.println("Authorization: " + auth);
//         client.println("Content-Type: multipart/form-data; boundary=AaB03x");
//         client.println("Content-Length: " + String(full_length));
//         client.println();
//         client.print(start_request);

//         while (file.available())
//         {
//             client.write(file.read());
//         }
//         client.println(end_request);
//         client.stop();
//         file.close();
//         Serial.println("File sent!");
//         Serial.println(">>><<<");
//     }
// }

#endif
