// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/MessagesV16/StopTransaction.h>
#include <ArduinoOcpp/MessagesV16/StartTransaction.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <ArduinoOcpp/Tasks/Metering/MeteringService.h>
#include <ArduinoOcpp/Debug.h>
#include <Arduino.h>
#include <ArduinoOcpp.h>
#include <ArduinoOcpp/MessagesV16/StatusNotification.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/OcppEvseState.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ConnectorStatus.h>

using ArduinoOcpp::Ocpp16::StopTransaction;
int meterStop = 0;
int meterStop1 = 0;
int meterStop2;
int totalmeterStop = 0;

// char Finishing = "Finishing";
StopTransaction::StopTransaction(int connectorId) : connectorId(connectorId)
{
}

const char *StopTransaction::getOcppOperationType()
{
    return "StopTransaction";
}

void StopTransaction::initiate()
{
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(4) + (JSONDATE_LENGTH + 1)));
    JsonObject payload = doc->to<JsonObject>();
    payload["status"] = "Accepted";
    if (ocppModel && ocppModel->getMeteringService())
    {
        auto meteringService = ocppModel->getMeteringService();
        meterStop = (int)meteringService->readEnergyActiveImportRegister(connectorId);
    }

    if (ocppModel)
    {
        otimestamp = ocppModel->getOcppTime().getOcppTimestampNow();
    }
    else
    {
        otimestamp = MIN_TIME;
    }

    if (ocppModel && ocppModel->getConnectorStatus(connectorId))
    {
        auto connector = ocppModel->getConnectorStatus(connectorId);
        connector->setTransactionId(-1); // immediate end of transaction
        if (connector->getSessionIdTag())
        {
            AO_DBG_DEBUG("Ending EV user session triggered by StopTransaction");
            connector->endSession();
        }
    }

    AO_DBG_INFO("StopTransaction initiated!");
    ocppsetup.lcdClear();
    ocppsetup.buzz();
    ocppsetup.lcdPrint("StopTransaction",0,0);
    ocppsetup.lcdPrint("initiated!",1,0);
    delay(1000);
    ocppsetup.lcdClear();
    //
    // OcppEvseState state = OcppEvseState::Finishing;
    payload["status"] = "Accepted";
    //
}

std::unique_ptr<DynamicJsonDocument> StopTransaction::createReq()
{
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(4) + (JSONDATE_LENGTH + 1)));
    JsonObject payload = doc->to<JsonObject>();
    // ocppsetup.lcdClear();
    // ocppsetup.lcdPrint("Do not Unplug your",0,0);
    // ocppsetup.lcdPrint("EV !!",1,0);
    ocppsetup_ocpp.ledChangeColour(255, 0, 0);
    // if(digitalRead(EV_Plug_Pin)==EV_Plugged)
    // {

    //     do{
    //     delay(500);
    //     ocppsetup_ocpp.ledChangeColour(255, 0, 0);
    //     ocppsetup.lcdClear();
    //     ocppsetup.lcdPrint("Please Unplug ", 0, 0);
    //     ocppsetup.lcdPrint("Your EV !! ", 1, 0);
    //     ocppsetup.lcdPrint("",1, 0);

    //     AO_DBG_INFO("PLease unplug your EV");
    //     delay(2000);
    //     }while(digitalRead(EV_Plug_Pin)!=EV_Unplugged);
    //     // return;
    //     // payload["meterStop"] = 0.0;//
    // }
    if (meterStop >= 0)
    {
        // Serial.println("meterStop :-");
        // Serial.println(meterStop);
        // Serial.println("meterStop1 :- ");
        // Serial.println(meterStop1);
        // Serial.println("meterStop2 :- ");
        // Serial.println(meterStop2);
        meterStop2 = meterStop - meterStop1;
        meterStop1 = meterStop;
        // Serial.println("meterStop1 after assign:- ");
        // Serial.println(meterStop1);
        payload["meterStop"] = meterStop2;
        Serial.println(meterStop2);
        char buffer[1000];
        sprintf(buffer, "%d", meterStop2);
        char *temp_value = buffer;
        ocppsetup_ocpp.lcdClear();
        ocppsetup_ocpp.lcdPrint("Meter Stoped At,", 0, 0);
        ocppsetup_ocpp.lcdPrint("( Wh )", 1, 0);
        ocppsetup_ocpp.lcdPrint(temp_value, 2, 0);
        delay(10000);
        ocppsetup_ocpp.lcdClear();
    }
    if (otimestamp > MIN_TIME)
    {
        char timestamp[JSONDATE_LENGTH + 1] = {'\0'};
        otimestamp.toJsonString(timestamp, JSONDATE_LENGTH + 1);
        payload["timestamp"] = timestamp;
    }

    if (ocppModel && ocppModel->getConnectorStatus(connectorId))
    {
        auto connector = ocppModel->getConnectorStatus(connectorId);
        payload["transactionId"] = connector->getTransactionIdSync();
    }
    return doc;
}


void StopTransaction::processConf(JsonObject payload)
{

    if (ocppModel && ocppModel->getConnectorStatus(connectorId))
    {
        auto connector = ocppModel->getConnectorStatus(connectorId);
        connector->setTransactionIdSync(-1);
    }

    AO_DBG_INFO("Request has been accepted!");
    digitalWrite(RELAY_1, HIGH);
    digitalWrite(RELAY_2, HIGH);
    if (digitalRead(EV_Plug_Pin) == EV_Plugged)
    {
    ocppsetup.buzz();
    // setstate("Finishing");
    
        ocppsetup_ocpp.lcdClear();
        do
        {
            delay(500);
            ocppsetup_ocpp.ledChangeColour(255, 0, 0);
            ocppsetup.lcdClear();
            ocppsetup.lcdPrint("Please Unplug ", 0, 0);
            ocppsetup.lcdPrint("Your EV !! ", 1, 0);
            ocppsetup.lcdPrint("", 1, 0);

            AO_DBG_INFO("PLease unplug your EV");
            delay(2000);
        } while (digitalRead(EV_Plug_Pin) != EV_Unplugged);
        // return OcppEvseState::Finishing;
        
    }
        // return 0;
        // payload["meterStop"] = 0.0;//
        delay(500);
        ocppsetup.buzz();
        ocppsetup_ocpp.lcdClear();
        ocppsetup_ocpp.lcdPrint("Charging Ended", 0, 0);
        ocppsetup_ocpp.lcdPrint("Thankyou for Using", 1, 0);
        ocppsetup_ocpp.lcdPrint("GridenPoint !", 2, 0);
        ocppsetup.lcdPrint("MTR: ", 3, 0);
        char *mtr;
        if (ocppsetup.getStatus() == "Available")
            mtr = "OK"; //"AVL";
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

        ocppsetup.lcdPrint(mtr, 3, 5);
        ocppsetup.lcdPrint("NTWK: ", 3, 10);
        const char *ntwk = ((WiFi.status() == WL_CONNECTED) ? "OK" : "ERR");
        ocppsetup.lcdPrint(ntwk, 3, 16);
        delay(10000);
        ocppsetup_ocpp.lcdClear();
    
}

void StopTransaction::processReq(JsonObject payload)
{
    /**
     * Ignore Contents of this Req-message, because this is for debug purposes only
     */
}

std::unique_ptr<DynamicJsonDocument> StopTransaction::createConf()
{
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(2 * JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();

    JsonObject idTagInfo = payload.createNestedObject("idTagInfo");
    idTagInfo["status"] = "Accepted";

    return doc;
}
