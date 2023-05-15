// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/MessagesV16/RemoteStartTransaction.h>
#include <ArduinoOcpp/MessagesV16/RemoteStopTransaction.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <ArduinoOcpp/Debug.h>
#include <ArduinoOcpp.h> //Matt-x/ArduinoOcpp library
#include <Arduino.h>     //Arduino firmware library
#include <ArduinoOcpp/MessagesV16/Authorize.h>
#include <iostream>
#include <ArduinoOcpp/MessagesV16/StatusNotification.h>
using ArduinoOcpp::Ocpp16::RemoteStartTransaction;
#include <LiquidCrystal_I2C.h>

// variables declaration
// bool transaction_in_process = false; //Check if transaction is in-progress
int evPlugged1 = 0; // Check if EV is plugged-in
bool transaction = false;
const char *idTag;
bool canStartTransaction = false;
signed trans_id = -1;
int transactionid;
RemoteStartTransaction::RemoteStartTransaction()
{
}

const char *RemoteStartTransaction::getOcppOperationType()
{
    return "RemoteStartTransaction";
}

void RemoteStartTransaction::processReq(JsonObject payload)
{
    connectorId = payload["connectorId"] | -1;
    transactionid = payload["transactionId"] | -1;
    const char *idTagIn = payload["idTag"] | "A0000000";
    size_t len = strnlen(idTagIn, IDTAG_LEN_MAX + 2);
    if (len <= IDTAG_LEN_MAX)
    {
        snprintf(idTag, IDTAG_LEN_MAX + 1, "%s", idTagIn);
    }

    if (payload.containsKey("chargingProfile"))
    {
        AO_DBG_WARN("chargingProfile via RmtStartTransaction now supported");
    }
}

std::unique_ptr<DynamicJsonDocument> RemoteStartTransaction::createConf()
{
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();

    if (*idTag == '\0')
    {
        AO_DBG_WARN("idTag format violation");
        payload["status"] = "Rejected";
        return doc;
    }

    if (connectorId >= 1)
    {
        // connectorId specified for given connector, try to start Transaction here
        if (ocppModel && ocppModel->getConnectorStatus(connectorId))
        {
            auto connector = ocppModel->getConnectorStatus(connectorId);
            if (connector->getTransactionId() < 0 &&
                connector->getAvailability() == AVAILABILITY_OPERATIVE &&
                connector->getSessionIdTag() == nullptr)
            {
                canStartTransaction = true;
            }
        }
    }
    else
    {
        // connectorId not specified. Find free connector
        if (ocppModel && ocppModel->getChargePointStatusService())
        {
            auto cpStatusService = ocppModel->getChargePointStatusService();
            for (int i = 1; i < cpStatusService->getNumConnectors(); i++)
            {
                auto connIter = cpStatusService->getConnector(i);
                if (connIter->getTransactionId() < 0 &&
                    connIter->getAvailability() == AVAILABILITY_OPERATIVE &&
                    connIter->getSessionIdTag() == nullptr)
                {
                    canStartTransaction = true;
                    connectorId = i;
                    break;
                }
            }
        }
    }

    if (canStartTransaction)
    {
        if (getTransactionId() <= 0)
        {
            if (ocppModel && ocppModel->getConnectorStatus(connectorId))
            {
                auto connector = ocppModel->getConnectorStatus(connectorId);
                evPlugged1 = 0;
                // connector->beginSession(idTag);
                if (digitalRead(EV_Plug_Pin) == EV_Plugged)
                {
                    connector->beginSession(idTag);
                    authorize(idTag, [](JsonObject response)
                              {
                    char idTag [IDTAG_LEN_MAX+1] = {'\0'};
                    Serial.print(evPlugged1);
                    if (!strcmp("Accepted", response["idTagInfo"]["status"] | "Invalid"))
                    {
                        Serial.println(F("User is authorized to start charging"));
                        delay(2000);
                        evPlugged1 = 1;
                        

                        startTransaction(idTag,[](JsonObject response){
                            Serial.printf("[remote] Started OCPP transaction. Status: %s, transactionId: %u\n",
                            response["idTagInfo"]["status"] | "Invalid",
                            response["transactionId"] | -1);

                            trans_id = (response["transactionId"] | -1);
                            
                        });
                    } });
                    ocppsetup.buzz();
                    payload["status"] = "Accepted";
                }
                else if (digitalRead(EV_Plug_Pin) == EV_Unplugged)
                {
                    evPlugged1 = 0;
                    ocppsetup.buzz();
                    Serial.println("Please Connect Charger first then Try Again");
                    payload["status"] = "Rejected";
                    // ocppsetup_ocpp.lcdClear();

                    // ocppsetup_ocpp.lcdClear();

                    // ocppsetup_ocpp.lcdPrint("Please Connect ", 0, 0);
                    // ocppsetup_ocpp.lcdPrint("Charger First then ", 1, 0);
                    // ocppsetup_ocpp.lcdPrint("Try Again ! ", 2, 0);

                    ocppsetup_ocpp.ledChangeColour(255, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(0, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(255, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(0, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(255, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(0, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(255, 0, 0);
                    delay(500);
                    ocppsetup_ocpp.ledChangeColour(0, 0, 0);
                    delay(500);
                    // ocppsetup_ocpp.lcdClear();
                    // Serial.printf("Reason: %s",payload["status"]);
                }
            }
            // payload["status"] = "Accepted";
        }
        /*else if(getTransactionId() == transactionid){
            Serial.println("Transaction with this ID Tag already running");
            AO_DBG_INFO("Transaction with this ID Tag already running");
            payload["status"] = "Rejected";
        }*/
        else
        {
            // Serial.println("No connector to start transaction");
            // AO_DBG_INFO("No connector to start transaction");
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
            Serial.println("No connector to start transaction");
            AO_DBG_INFO("No connector to start transaction");
            payload["status"] = "Rejected";
        }
    }

    // code below works fine in that first request is sent then plug pin is connected(5v)
    /*if (canStartTransaction){

            if (ocppModel && ocppModel->getConnectorStatus(connectorId)) {
            auto connector = ocppModel->getConnectorStatus(connectorId);
            evPlugged1 = 0;
            connector->beginSession(idTag);
            authorize(idTag, [](JsonObject response){
                char idTag [IDTAG_LEN_MAX + 1] = {'\0'};

                Serial.println(evPlugged1);

                if (!strcmp("Accepted", response["idTagInfo"]["status"] | "Invalid")){
                    Serial.println(F("User is authorized to start charging"));
                    delay(9000);
                    if(digitalRead(EV_Plug_Pin)==EV_Plugged && evPlugged1 == 0 && isAvailable()){
                        evPlugged1 = 1;
                        startTransaction(idTag, [] (JsonObject response) {
                       //Callback: Central System has answered. Could flash a confirmation light here.

                        Serial.printf("[remote] Started OCPP transaction. Status: %s, transactionId: %u\n",
                        response["idTagInfo"]["status"] | "Invalid",
                        response["transactionId"] | -1);

                        trans_id = (response["transactionId"] | -1);

                        });

                    delay(2000);
                    Serial.println(evPlugged1);*/
    /*if(evPlugged1==1){

        Serial.println("Hey");
        do
            {
                Serial.println("\n Inside loop \n");
                if (digitalRead(EV_Plug_Pin) == EV_Unplugged)
                {
                    stopTransaction([] (JsonObject response){
                    Serial.print("Stopped OCPP transaction");
                    evPlugged1 = 0;
                    });
                    trans_id = -1;
                    break;
                }
            }while (evPlugged1==0);
    }*/

    /*}
}//
});
}
payload["status"] = "Accepted";
//abort();
}*/ else
    {
        AO_DBG_INFO("No connector to start transaction");
        // payload["status"] = "Accepted";
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

    if (payload["status"] == "Accepted" && evPlugged1 == 1)
    {
        Serial.println("Hey");
        do
        {
            Serial.println("\n Inside loop \n");
            if (digitalRead(EV_Plug_Pin) == EV_Unplugged)
            {
                stopTransaction([](JsonObject response)
                                {
                                    Serial.print("Stopped OCPP transaction");
                                    evPlugged1 = 0; });
                trans_id = -1;
                break;
            }
        } while (evPlugged1 == 0);
    }
    return doc;
}

std::unique_ptr<DynamicJsonDocument> RemoteStartTransaction::createReq()
{
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();

    payload["idTag"] = "A0-00-00-00";

    return doc;
}

void RemoteStartTransaction::processConf(JsonObject payload)
{
}

void st()
{
    Serial.println("\n hii");
    if (evPlugged1 == 1)
    {
        Serial.println("inside if");
        do
        {
            Serial.print("inside loop");
            if (digitalRead(EV_Plug_Pin) == EV_Unplugged)
            {
                stopTransaction([](JsonObject response)
                                {
                                Serial.print("Stopped OCPP transaction");
                                evPlugged1 = 0; });
            }
        } while (evPlugged1 == 0);
    }
}
