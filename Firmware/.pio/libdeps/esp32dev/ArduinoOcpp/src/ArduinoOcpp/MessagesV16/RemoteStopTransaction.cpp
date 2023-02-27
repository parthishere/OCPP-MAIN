// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/MessagesV16/RemoteStopTransaction.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <ArduinoOcpp/Debug.h>
#include <ArduinoOcpp.h> //Matt-x/ArduinoOcpp library
#include <Arduino.h> //Arduino firmware library
#include <ArduinoOcpp/MessagesV16/Authorize.h>
#include <ArduinoOcpp/MessagesV16/StatusNotification.h>


//const char *cstrFromOcppEveState(OcppEvseState state);
//bool transaction_in_process = false; //Check if transaction is in-progress

using ArduinoOcpp::Ocpp16::RemoteStopTransaction;

RemoteStopTransaction::RemoteStopTransaction() {
  
}

const char* RemoteStopTransaction::getOcppOperationType(){
    return "RemoteStopTransaction";
}

void RemoteStopTransaction::processReq(JsonObject payload) {
    transactionId = payload["transactionId"] | -1;
}

std::unique_ptr<DynamicJsonDocument> RemoteStopTransaction::createConf(){
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();
    
    bool canStopTransaction = false;

    if (ocppModel && ocppModel->getChargePointStatusService()) {
        auto cpStatusService = ocppModel->getChargePointStatusService();
        for (int i = 0; i < cpStatusService->getNumConnectors(); i++) {
            auto connIter = cpStatusService->getConnector(i);
            if (connIter->getTransactionId() == transactionId) {
                canStopTransaction = true;
                connIter->endSession();
            }
        }
    }

    if (canStopTransaction){
        payload["status"] = "Accepted";
        int a,b;
        a=getTransactionId();
        Serial.println(a);
        Serial.println(transactionId);
        if(a==transactionId){
            payload["status"] = "Accepted";
            stopTransaction([](JsonObject response  ){
                //const char *stat = "Finishing";
                Serial.print(F("Stopping OCPP transaction from remote\n"));
                /*if(digitalRead(EV_Plug_Pin)==EV_Plugged)
                {
                    //(OcppEvseState::Finishing);
                    do
                    {
                        Serial.print("\n PLease Unplug your EV");
                        //JsonObject response = "Finishing"; 
                        delay(1000);
                    }while(digitalRead(EV_Plug_Pin)==EV_Plugged);
                }*/
                Serial.print("\n Stopped OCPP transaction from remote");
                //(OcppEvseState::Available);
            });
        }
        else{
            AO_DBG_ERR("Invalid OCPP transactionID");
            payload["status"]="Rejected";
            Serial.println("\n Invalid OCPP transactionID");

        }
        

        /*int count = 0;
        do
        {
        if(digitalRead(EV_Plug_Pin)==EV_Plugged)
        {
            //stopTransaction([] (JsonObject response){
                //(OcppEvseState::Finishing);
                //(OcppEvseState state) = "Finishing";
            //});
            Serial.println(payload);
            Serial.println("\n PLease Unplug");
            count = 1;
            delay(2000);
        }   
        else if(digitalRead(EV_Plug_Pin)==EV_Unplugged){
            payload["status"] = "Accepted";
            stopTransaction([] (JsonObject response) {
                //Callback: Central System has answered.
                Serial.print(F("Stopped OCPP transaction from remote\n"));
                });
                count =0;
                //isAvailable() = true;
                break;
        }
        //isAvailable() = true;
        } while (count!=0);*/
        
        
        /*payload["status"] = "Accepted";
        stopTransaction([] (JsonObject response) {
                //Callback: Central System has answered.
                Serial.print(F("Stopped OCPP transaction from remote\n"));
                });*/
        /*if ((EV_Plug_Pin) == EV_Plugged)
        {
            Serial.print("Unplug EV");
        }*/
        //transaction_in_process = false;
    /*} else {
        payload["status"] = "Rejected";
    }
    */
    isInSession() == true;
    return doc;
}
}
