// Including Libraries

#include <Arduino.h> //Arduino firmware library
#include <WiFi.h>    //Wifi library
// #include <ZMPT101B.h>
// #include <ACS712.h>
#include <ArduinoOcpp.h> //Matt-x/ArduinoOcpp library
#include <ArduinoOcpp/Debug.h>
#include <MFRC522.h> //MFRC522 library
#include <SPI.h>     //Include SPI library
// #include <Firmware/lib/OcppSetup/OcppSetup.h>
#include "OcppSetup.h"
#include <ArduinoOcpp/MessagesV16/ChangeAvailability.h>
#include <ArduinoOcpp/MessagesV16/StatusNotification.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ConnectorStatus.h>
#include <ArduinoOcpp/Core/OcppEngine.h>
#include <ArduinoOcpp/MessagesV16/TriggerMessage.h>
#include<LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// WiFi connnection details
#define SSID "ALIENWARE"
#define Password "He@ven$heth05"

// Webserver details
#define OCPP_HOST "192.168.137.1"//"ocpp.gridenpower.com"//"65.2.90.45" //"3.109.235.50" //"3.110.42.33" // //
#define OCPP_PORT 6630
#define OCPP_URL "ws://192.168.137.1:6630/ocpp/GP001"//"ws://ocpp.gridenpower.com:6630/ocpp/GP004"//"ws://65.2.90.45:6630/ocpp/CKSS0999" //"ws://3.109.235.50:6630/ocpp/GP004" //"ws://3.110.42.33:6630/ocpp/GP004"

#define FILE_SERVER_URL "https://fileuploads.up.railway.app"

// RFid data read
#define SDA_SS_PIN 5 // 21 //ESP Interface Pin
#define RST_PIN 15   // 22    //ESP Interface Pin
#define LOG_BTN_PIN 13

#define RELAY_1 14
#define RELAY_2 27

#define RledPin 33
#define BledPin 32 // 27 corresponds to GPIO2
#define GledPin 25

MFRC522 mfrc522(SDA_SS_PIN, RST_PIN); // create instance of class
MFRC522::MIFARE_Key key;
// LiquidCrystal_I2C lcd (0x27, 16,2);
// ZMPT101B voltageSensor(26);
// Pin Mapping
#define Amperage_Pin 4 // modulated as PWM // ACS712

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
#define SOS_pin 12
#define SOS_pressed LOW
#define SOS_unpressed HIGH
#define volt_pin 26 //ZMPT101B

void chargetime();
// variables declaration

bool transaction_in_process = false; // Check if transaction is in-progress
int evPlugged = EV_Unplugged;        // Check if EV is plugged-in

bool booted = false;
ulong scheduleReboot = 0;   // 0 = no reboot scheduled; otherwise reboot scheduled in X ms
ulong reboot_timestamp = 0; // timestamp of the triggering event; if scheduleReboot=0, the timestamp has no meaning
extern String content = "";
extern String content2 = "";
extern signed tran_id = -1;
char st;
int screen = 0;
// int check = 0;
/*
 * Typical pin layout used:
 * ---------------------------------
 *            MFRC522      ESP32
 *            Reader/PCD
 *Signal      Pin          Pin
 * ----------------------------------
 *RST/Reset   RST          15
 *SPI SS      SDA(SS)      5
 *SPI MOSI    MOSI         23
 *SPI MISO    MISO         19
 *SPI SCK     SCK          18
 *Relay 1                  14
 *Relay 2                  27
 *EV_plug pin              36
 *EV_charge pin            35
 *SOS_pin                  12
*/

// Setup Loop

OcppSetup ocppsetup;
ArduinoOcpp::Ocpp16::ChangeAvailability changeAva;
int HH = 0, MM = 0, S = 0;
int freq = 5000;
int RledChannel = 1;
int GledChannel = 2;
int BledChannel = 3;
int resolution = 8;
long int prev_millis;
long int millissec;
// bool connection_EV = false;
// bool charge_EV = false;
char previousstate;
void setup()
{

    Serial.begin(115200); // set baud rate
    Serial.setDebugOutput(true);
    // Signals Pin Mode Declaration
    pinMode(EV_Plug_Pin, INPUT);
    pinMode(EV_Charge_Pin, INPUT);
    pinMode(EVSE_Ground_Fault_Pin, INPUT);
    pinMode(OCPP_Charge_Permission_Pin, OUTPUT);
    digitalWrite(OCPP_Charge_Permission_Pin, OCPP_Charge_Forbidden);
    pinMode(OCPP_Availability_Pin, OUTPUT);
    digitalWrite(OCPP_Availability_Pin, OCPP_Unavailable);

    pinMode(BUZZER_PIN, OUTPUT);

    // ocppsetup.lcdInitialize();
    ocppsetup.touchSetup();
    ledcSetup(RledChannel, 1000, 8);
    ledcSetup(GledChannel, 1000, 8);
    ledcSetup(BledChannel, 1000, 8);

    ledcAttachPin(RledPin, RledChannel);
    ledcAttachPin(GledPin, GledChannel);
    ledcAttachPin(BledPin, BledChannel);
    pinMode(SOS_pin, INPUT_PULLUP); // SOS button
    prev_millis = millis();
    millissec = millis();
    // pinMode(CHARGE_PERMISSION_LED, OUTPUT);
    // digitalWrite(CHARGE_PERMISSION_LED, CHARGE_PERMISSION_OFF);
    pinMode(volt_pin,INPUT);
    pinMode(Amperage_Pin, INPUT);
    // pinMode(Amperage_Pin, OUTPUT);
    ledcSetup(0, 1000, 8); // channel=0, freq=1000Hz, range=(2^8)-1
    ledcAttachPin(Amperage_Pin, 0);
    ledcWrite(Amperage_Pin, 256); // 256 is constant +3.3V DC

    /*------------------------------Initializing Wifi connection ---------------------------*/
    // Wifi connection procedure
    Serial.print(F("Wait for Wifi to connect : "));
    // ocppsetup.lcdClear();
    // ocppsetup.lcdPrint("Waiting for Wifi to connect");

    WiFi.begin(SSID, Password);

    while (!WiFi.isConnected())
    { // Wait for Wifi to connect
        Serial.print("..");
        delay(1000);
    }
    // ocppsetup.lcdClear();
    // ocppsetup.lcdPrint("Connected to WiFi", 0, 0);

    ocppsetup.buzz();
    /*------------------------------Wifi Connection Over ---------------------------*/

    /*-----------------------Initialize the OCPP library---------------------------*/
    OCPP_initialize(OCPP_HOST, OCPP_PORT, OCPP_URL);

    /* Integrate OCPP functionality. You can leave out the following part if your EVSE doesn't need it.*/

    setEnergyActiveImportSampler([]()
                                 {
    //read the energy input register of the EVSE here and return the value in Wh
    /** Approximated value. TODO: Replace with real reading**/
    static ulong lastSampled = millis();
    static float energyMeter = 0.f;
    if (getTransactionId() > 0 && digitalRead(EV_Charge_Pin) == EV_Charging)
        energyMeter += ((float) millis() - lastSampled) * 0.003f; //increase by 0.003Wh per ms (~ 10.8kWh per h)
    lastSampled = millis();
    return energyMeter; });

    /*setOnChargingRateLimitChange([](float limit) {
      //set the SAE J1772 Control Pilot value here
      Serial.print(F("[main] Smart Charging allows maximum charge rate: "));
      Serial.println(limit);
      float amps = limit / 230.f;
      if (amps > 51.f)
          amps = 51.f;

      int pwmVal;
      if (amps < 6.f) {
          pwmVal = 256; // = constant +3.3V DC
      } else {
          pwmVal = (int) (4.26667f * amps);
      }
      ledcWrite(Amperage_Pin, pwmVal);

    });*/

    setEvRequestsEnergySampler([]()
                               {
    //return true if the EV is in state "Ready for charging" (see https://en.wikipedia.org/wiki/SAE_J1772#Control_Pilot)
    //return false;
    return digitalRead(EV_Charge_Pin) == EV_Charging; });

    addConnectorErrorCodeSampler([]()
                                 {
                                     // if (digitalRead(EVSE_GROUND_FAULT_PIN) != EVSE_GROUND_CLEAR) {
                                     // return "GroundFault";
                                     // } else {
                                     return (const char *)NULL;
                                     // }
                                 });

    setOnResetSendConf([](JsonObject payload)
                       {
        if (getTransactionId() >= 0)
            stopTransaction();
        
        reboot_timestamp = millis();
        scheduleReboot = 5000;
        booted = false; });

    // Check connector
    setOnUnlockConnector([]()
                         { return true; });

    /*------------Notify the Central System that this station is ready--------------*/
    /*-----------------------------BOOT NOTIFICATION---------------------------------*/

    bootNotification("EV CHARGER", "GRIDEN POWER", [](JsonObject payload)
                     {
    const char *status = payload["status"] | "INVALID";
    if (!strcmp(status, "Accepted")) {
        booted = true;
        Serial.println("Sever Connected!");
        ocppsetup.buzz();

        //digitalWrite(SERVER_CONNECT_LED, SERVER_CONNECT_ON);
    } else {
        //retry sending the BootNotification
        ocppsetup.buzz();
        delay(60000); 
        ESP.restart();
        
    } });

    /*---------------------Initializing MFRC522 RFID Library------------------------*/
    SPI.begin();        // Initiate  SPI bus
    mfrc522.PCD_Init(); // Initiate MFRC522
    // Serial.println("Please verify your RFID tag...");

    pinMode(LOG_BTN_PIN, INPUT_PULLUP);
    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, 1);

    pinMode(RELAY_2, OUTPUT);
    digitalWrite(RELAY_2, 1);
}

// Continous loop
bool was_available, was_charging;

bool initial_push = false;
bool sent = false;
void loop()
{

    const char *currentstate;
    currentstate = ocppsetup.getStatus();
    char currentstate1 = *currentstate;
    
    JsonObject payload;
    // if (initial_push == true)
    // {
    //     initial_push = false;
        // ocppsetup.sendCSVFile("parthishere:123");
    //     sent = true;
    // }
    // // if (digitalRead(LOG_BTN_PIN) == 0 && sent == false)
    // if (previousstate != currentstate1 && sent == false)
    // {
    //     // *previousstate = *currentstate;
    //     Serial.println("Server button pressed");
    //     initial_push = true;
    //     previousstate = currentstate1;
    // }
    // if (digitalRead(LOG_BTN_PIN) == 1)
    // {
    //     sent = false;
    // }   

    OCPP_loop();
    // reboot esp after 3 minutes
    // if (millis() - millissec > 200000)
    // {
    //     if(ocppsetup.getStatus() == "Available")
    //     {
    //         ocppsetup.lcdClear();
    //         while (digitalRead(EV_Plug_Pin) == EV_Plugged)
    //         {
    //             Serial.print(".");
    //             ocppsetup.lcdPrint("Please Unplug your EV!!",0,0);
    //             delay(5000);
    //         }
    //         ocppsetup.lcdClear();
    //         millissec = millis();
    //         ESP.restart();
    //     }
    //     else{
    //     millissec = millis();
    //     }
    // }
    if (millis() - prev_millis > 500)//display meter settings.
    {
        char *mtr;
        if (ocppsetup.getStatus() == "Available")
            mtr = "OKY"; //"AVL";
        // else if (ocppsetup.getStatus() ==  "Preparing") mtr = "OK";//"PRE";
        // else if (ocppsetup.getStatus() ==  "Charging") mtr = "OK";//"CHR";
        // else if (ocppsetup.getStatus() ==  "Finishing") mtr = "OK";//"FIN";
        // else if (ocppsetup.getStatus() ==  "Preparing") mtr = "OK";//"PRE";
        else if (ocppsetup.getStatus() == "SuspendedEVSE")
            mtr = "ERR"; //"S-EVSE";
        // else if (ocppsetup.getStatus() ==  "SuspendedEV") mtr = "OK";//"S-EV";
        // else if (ocppsetup.getStatus() ==  "Reserved") mtr = "OK";//"RES";
        else if (ocppsetup.getStatus() == "Unavailable")
            mtr = "ERR"; //"UNA";
        else if (ocppsetup.getStatus() == "Faulted")
            mtr = "ERR"; //"FAULT";
        // ocppsetup.lcdPrint(mtr, 3, 5);
        // ocppsetup.lcdPrint("NTWK: ", 3, 10);
        const char *ntwk = ((WiFi.status() == WL_CONNECTED) ? "OK" : "ERR");
        // ocppsetup.lcdPrint(ntwk, 3, 16);
        prev_millis = millis();
    }
    if (ocppsetup.getStatus() == "Available") // display welcome message.
    {
        // ocppsetup.lcdPrint("Welcome To", 0, 2);
        // ocppsetup.lcdPrint("Griden Point!!!!", 1, 0);
        // ocppsetup.lcdPrint("MTR: ", 3, 0);
        // ocppsetup.lcdPrint("                    ", 2, 0);
        ocppsetup.dwin_state(AVL_GREEN);
        ocppsetup.dwin_qr(1);
        ocppsetup.ledChangeColour(0, 0, 255);
        was_available = true;
        delay(500);
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2, HIGH);
        HH = 0;
        MM = 0;
        S = 0;
    }
    else if (ocppsetup.getStatus() == "Charging") //charging condition and relay turning on.
    {
        ocppsetup.ledChangeColour(0, 255, 0);
        ocppsetup.dwin_state(CHAR_GREEN);
        // ocppsetup.lcdPrint("Charging Started", 0, 2);
        // ocppsetup.lcdPrint("Time : ", 2, 0);
        // ocppsetup.lcdPrint(payload["timestamp"], 2, 7);
        was_available = false;
        was_charging = true;
        
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
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
    }
    else if (ocppsetup.getStatus() == "SuspendedEVSE" || ocppsetup.getStatus() == "SuspendedEV" || ocppsetup.getStatus() == "Faulted" || ocppsetup.getStatus() == "available")
    {
        if (ocppsetup.getStatus() != "Available")
            ocppsetup.ledChangeColour(255, 255, 0);
        digitalWrite(RELAY_1, HIGH);
        digitalWrite(RELAY_2, HIGH);
    }
    else if (ocppsetup.getStatus() == "Unavailable" && digitalRead(SOS_pin) == SOS_pressed)//sos button pressed condition
    {
        ocppsetup.ledChangeColour(255, 0, 0);
        payload["status"] = "Rejected";
        transaction_in_process = false;
        tran_id = -1;
        evPlugged = EV_Unplugged;
        // ocppsetup.lcdClear();
    }
    // To clear the screen after every transaction
    if (ocppsetup.getStatus() == "Available" && screen == 0)
    {
        Serial.println("inside screen clear after stop transaction");
        // ocppsetup.lcdClear();
        screen = 1;
    }
    if (ocppsetup.getStatus() == "Charging" && screen == 1)
    {
        Serial.println("Inside screen clear after starting charging");
        // ocppsetup.lcdClear();
        screen = 0;
    }
    // showing charge time on LCD screen.
    // if(ocppsetup.getStatus() == "Charging")
    // {
    //     S++;  
    //     delay ( 1000 );  
    //     if ( S > 59 )  
    //     {  
    //         MM++;  
    //         S = 0;  
    //     }  
    //     if ( MM > 59 )  
    //     {  
    //         HH++;  
    //         MM = 0;  
    //     }  
    //     // Serial.printf("\n %d : %d : %d",HH,MM,S);
    //     ocppsetup.lcdPrintint(HH, 2,7);
    //     ocppsetup.lcdPrint(":", 2,9);
    //     ocppsetup.lcdPrintint(MM, 2,10);
    //     ocppsetup.lcdPrint(":", 2,12);
    //     ocppsetup.lcdPrintint(S,2,13);
    // }
    // if(digitalRead(EV_Charge_Pin)==EV_Plugged && charge_EV == false)
    // {
    //     Serial.println("Setting status as prepering");

    // }

    // Reset Condition
    // loop when remote stop is executed
    if (isInSession() == false /*true*/ && transaction_in_process == true /*true*/ && evPlugged == EV_Plugged && digitalRead(EV_Plug_Pin) == EV_Plugged)
    {
        Serial.println("Reset");
        ocppsetup.buzz();
        ocppsetup.buzz();
        while (digitalRead(EV_Plug_Pin) == EV_Plugged)
        {
            Serial.print(".");
            delay(5000);
        }
        if (isAvailable())
        {

            ESP.restart();
        }

        transaction_in_process = false;
        evPlugged = EV_Unplugged;
        tran_id = 0;
    }

    // If RIFD chip is detected process for verification
    if (!mfrc522.PICC_IsNewCardPresent() && transaction_in_process == false)
    {
        return;
    }
    // Verify if the NUID has been readed
    else if (!mfrc522.PICC_ReadCardSerial() && transaction_in_process == false)
    {
        Serial.println("Card not readed");
        // ocppsetup.lcdClear();
        ocppsetup.buzz();
        // ocppsetup.lcdPrint("Card not readed");
        ocppsetup.dwin_rfid(false);
        return;
    }
    else if (transaction_in_process == false) // && isInSession() == false)
    {
        // Show UID on serial monitor
        String content = "";
        MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
        Serial.println(mfrc522.PICC_GetTypeName(piccType));
        // Check is the PICC of Classic MIFARE type
        if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
            piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
            piccType != MFRC522::PICC_TYPE_MIFARE_4K)
        {
            Serial.println(F("Your tag is not of type MIFARE Classic."));
            ocppsetup.buzz();
            // ocppsetup.lcdClear();
            // ocppsetup.lcdPrint("Your tag is not of", 0, 0);
            // ocppsetup.lcdPrint("type MIFARE Classic.", 1, 0);
            return;
        }

        Serial.println("");

        Serial.print(F("The ID tag no is  : "));

        for (byte i = 0; i < mfrc522.uid.size; i++)
        {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : "");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
            content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : ""));
            content.concat(String(mfrc522.uid.uidByte[i], HEX));
        }

        Serial.println("");
        content2 = content;
        content.toUpperCase();
        char *idTag = new char[content.length() + 1];
        strcpy(idTag, content.c_str());

        char *temp = idTag;
        delay(500);
        if (getTransactionId() <= 0)
        {
            if (digitalRead(EV_Plug_Pin) == EV_Plugged)
            {
                // ocppsetup.lcdClear();
                ocppsetup.buzz();
                // ocppsetup.lcdPrint("Card Readed ! ", 0, 1);
                // ocppsetup.lcdPrint("Authorizing... ", 1, 0);
                delay(500);
                // ocppsetup.lcdClear();
                authorize(idTag, [](JsonObject response)
                {
                //check if user with idTag is authorized
                    if (!strcmp("Accepted", response["idTagInfo"]["status"] | "Invalid")){
                    ocppsetup.buzz();
                    // ocppsetup.lcdClear();
                    // ocppsetup.lcdPrint("Card Accepted", 0, 3);
                    // ocppsetup.lcdPrint("",1, 0);
                    // ocppsetup.lcdPrint("User is Authorized", 2, 0);
                    // ocppsetup.lcdPrint("to Start Charging", 3, 0);
                    ocppsetup.dwin_rfid(true);
                    ocppsetup.dwin_main(AUTHENTICATED_MAIN);
                    Serial.println(F("[main] User is authorized to start charging"));
                    ocppsetup.ledChangeColour(0, 255, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 0, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 255, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 0, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 255, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 0, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 255, 0);
                    delay(500);
                    ocppsetup.ledChangeColour(0, 0, 0);
                    transaction_in_process = true;
                    delay(500);
                    // ocppsetup.lcdClear();
                    
                    } else {
                        
                
                        Serial.printf("[main] Authorize denied. Reason: %s\n", response["idTagInfo"]["status"] | "");
                        // char *st =response["status"];
                        ocppsetup.buzz();
                        // ocppsetup.lcdClear();
                        
                        // ocppsetup.lcdPrint("Card Denied", 0, 5);
                        // ocppsetup.lcdPrint("",1, 0);
                        // ocppsetup.lcdPrint("Status ", 2, 0);
                        // ocppsetup.lcdPrint(st, 3, 0);
                        ocppsetup.dwin_rfid(false);
                        ocppsetup.dwin_main(UNAUTHENTICATED_MAIN);
                        ocppsetup.ledChangeColour(255, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(0, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(255, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(0, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(255, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(0, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(255, 0, 0);
                        delay(500);
                        ocppsetup.ledChangeColour(0, 0, 0);
                        delay(500);
                        // ocppsetup.lcdClear();
                        } });

                Serial.printf("[main] Authorizing user with idTag %s\n", idTag);

                delay(2000);

                if (ocppPermitsCharge())
                {
                    ocppsetup.buzz();
                    Serial.println("OCPP Charge Permission Forbidden");
                    delay(2000);
                }
                else
                {
                    ocppsetup.buzz();
                    Serial.println("OCPP Charge Permission Permitted");
                    delay(2000);
                }

                if (isInSession() == true)
                {
                    transaction_in_process = true;
                }
            }
            else
            {
                // ocppsetup.lcdClear();
                // ocppsetup.lcdPrint("Please Plug in your", 0, 0);
                // ocppsetup.lcdPrint("EV then try again!", 1, 0);
                ocppsetup.ledChangeColour(255, 0, 0);
                AO_DBG_INFO("PLease plug in your EV then try again!");
                payload["status"] = "Rejected";
                ocppsetup.buzz();
                delay(2000);
                // ocppsetup.lcdClear();
                return;
            }
        }
        else
        {
            JsonObject payload;
            Serial.println(F("[main] User is not authorized to start charging reason: Transaction in process"));
            AO_DBG_INFO("No connector to start transaction");
            ocppsetup.buzz();
            // ocppsetup.lcdClear();

            // ocppsetup.lcdPrint("No Connector ", 0, 0);
            // ocppsetup.lcdPrint("is Available", 1, 0);
            // ocppsetup.lcdPrint("to Start Charging!!", 2, 0);

            ocppsetup.ledChangeColour(255, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(0, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(255, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(0, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(255, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(0, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(255, 0, 0);
            delay(500);
            ocppsetup.ledChangeColour(0, 0, 0);
            delay(500);

            // ocppsetup.lcdClear();
            payload["status"] = "Rejected";
        }
    }
    else
    {
        delay(2000);
        if (!booted)
            return;
        if (scheduleReboot > 0 && millis() - reboot_timestamp >= scheduleReboot)
        {
            ocppsetup.buzz();
            ESP.restart();
        }
        if (digitalRead(EV_Plug_Pin) == EV_Plugged && evPlugged == EV_Unplugged && getTransactionId() >= 0)
        {
            // transition unplugged -> plugged; Case A: transaction has already been initiated
            evPlugged = EV_Plugged;
            Serial.println("In Case A");
        }
        else if (digitalRead(EV_Plug_Pin) == EV_Plugged && evPlugged == EV_Unplugged && isAvailable())
        {
            // transition unplugged -> plugged; Case B: no transaction running; start transaction
            // Serial.println("Case B: no transaction running; start transaction");
            isInSession() == true;
            evPlugged = EV_Plugged;
            content2.toUpperCase();
            char *idTag = new char[content2.length() + 1];
            strcpy(idTag, content2.c_str());
            startTransaction(idTag, [](JsonObject response)
                            {
                // Callback: Central System has answered. Could flash a confirmation light here.

                Serial.printf("[main] Started OCPP transaction. Status: %s, transactionId: %u\n",
                            response["idTagInfo"]["status"] | "Invalid",
                            response["transactionId"] | -1);
                st = response["status"];
                tran_id = (response["transactionId"] | -1); });
            ocppsetup.buzz();
            screen = 1;
            delay(500);
        }
        //--------------- Stop With RFid Auth --------------------//
        else if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() && evPlugged == EV_Plugged && transaction_in_process == true)
        {
            String content3 = "";
            for (byte i = 0; i < mfrc522.uid.size; i++)
            {
                Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : "");
                Serial.print(mfrc522.uid.uidByte[i], HEX);
                content3.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : ""));
                content3.concat(String(mfrc522.uid.uidByte[i], HEX));
            }

            content3.toUpperCase();
            Serial.println(content3);
            ocppsetup.buzz();
            if (content2 != content3 && getTransactionId >= 0)
            {
                Serial.println("No connector to start transaction");
                AO_DBG_INFO("No connector to start transaction");
                // ocppsetup.lcdClear();

                // ocppsetup.lcdPrint("No Connector ", 0, 0);
                // ocppsetup.lcdPrint("is Available", 1, 0);
                // ocppsetup.lcdPrint("to Start Charging!!", 2, 0);

                ocppsetup.ledChangeColour(255, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(0, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(255, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(0, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(255, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(0, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(255, 0, 0);
                delay(500);
                ocppsetup.ledChangeColour(0, 0, 0);
                delay(500);
                payload["status"] = "Rejected";
                // ocppsetup.lcdClear();
                return;
            }
            else
            {
                Serial.println("Card Authenticated.");

                if (tran_id >= 0)
                {
                    screen = 0;
                    stopTransaction([](JsonObject response)
                                    {
                                        // Callback: Central System has answered.
                                        Serial.print(F("[main] Stopped OCPP transaction\n")); });
                    Serial.println("Done");
                    digitalRead(EV_Plug_Pin) == EV_Unplugged;
                    transaction_in_process = false;
                    tran_id = -1;
                    evPlugged = EV_Unplugged;
                }
            }
        }
        else if (digitalRead(EV_Plug_Pin) == EV_Unplugged && evPlugged == EV_Plugged)
        { //  && isInSession() == true) {
            // transition plugged -> unplugged
            screen = 0;
            evPlugged = EV_Unplugged;
            if (tran_id >= 0)
            {
                ocppsetup.buzz();
                stopTransaction([](JsonObject response)
                                {
                    //Callback: Central System has answered.
                    Serial.print(F("[main] Stopped OCPP transaction\n")); });

                transaction_in_process = false;
                tran_id = -1;
            }
        }
    }
}
