// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/Core/OcppSocket.h>
#include <ArduinoOcpp/Core/OcppServer.h>
#include <ArduinoOcpp/Debug.h>
#include "ArduinoOcpp.h"

#ifndef AO_CUSTOM_WS
int disconnecttime = 0;
using namespace ArduinoOcpp;

using namespace ArduinoOcpp::EspWiFi;

OcppClientSocket::OcppClientSocket(WebSocketsClient *wsock) : wsock(wsock) {

}

void OcppClientSocket::loop() {
    wsock->loop();
}

bool OcppClientSocket::sendTXT(std::string &out) {
    return wsock->sendTXT(out.c_str(), out.length());
}

void OcppClientSocket::setReceiveTXTcallback(ReceiveTXTcallback &callback) {
    wsock->onEvent([callback](WStype_t type, uint8_t * payload, size_t length) {
        
        switch (type) {
            case WStype_DISCONNECTED:
                ocppsetup_ocpp.ServerDisconnect();
                // ocppsetup_ocpp.lcdClear();
                // ocppsetup_ocpp.lcdPrint("Disconnected from",0,0);
                // ocppsetup_ocpp.lcdPrint("SERVER",1,0);
                ocppsetup.dwin_server_wifi(1);
                ocppsetup.buzz();
                delay(1000);
                // ocppsetup_ocpp.lcdClear();
                ocppsetup_ocpp.setServerStatus("ERR");
                disconnecttime++;
                AO_DBG_INFO("Disconnected");
                break;
            case WStype_CONNECTED:
                AO_DBG_INFO("Connected to url: %s", payload);
                ocppsetup_ocpp.ServerConnect();
                // ocppsetup_ocpp.lcdClear();
                // ocppsetup_ocpp.lcdPrint("Connected to Server");
                ocppsetup.dwin_server_wifi(0);
                ocppsetup.buzz();
                delay(1000);
                // ocppsetup_ocpp.lcdClear();
                ocppsetup_ocpp.setServerStatus("CNT");
                disconnecttime = 0;
                break;
            case WStype_TEXT:
                AO_DBG_TRAFFIC_IN(payload);

                if (!callback((const char *) payload, length)) { //forward message to OcppEngine
                    AO_DBG_WARN("Processing WebSocket input event failed");
                }
                break;
            case WStype_BIN:
                AO_DBG_WARN("Binary data stream not supported");
                break;
            case WStype_PING:
                // pong will be send automatically
                AO_DBG_TRAFFIC_IN("WS ping");
                break;
            case WStype_PONG:
                // answer to a ping we send
                AO_DBG_TRAFFIC_IN("WS pong");
                break;
            case WStype_FRAGMENT_TEXT_START: //fragments are not supported
            default:
                AO_DBG_WARN("Unsupported WebSocket event type");
                break;
        }
        if(disconnecttime == 5)
        {
            if(ocppsetup.getStatus()=="Charging")
            {
                    stopTransaction([](JsonObject response)
                                {
                    //Callback: Central System has answered.
                    Serial.print(F("Stopped OCPP transaction due to server disconnect\n")); });
            }
        }
    });
}

OcppServerSocket::OcppServerSocket(IPAddress &ip_addr) : ip_addr(ip_addr) {
    
}

OcppServerSocket::~OcppServerSocket() {
    OcppServer::getInstance()->removeReceiveTXTcallback(this->ip_addr);
}

void OcppServerSocket::loop() {
    //nothing here. The client must call the EspWiFi server loop function
}

bool OcppServerSocket::sendTXT(std::string &out) {
    AO_DBG_TRAFFIC_OUT(out.c_str());
    return OcppServer::getInstance()->sendTXT(ip_addr, out);
}

void OcppServerSocket::setReceiveTXTcallback(ReceiveTXTcallback &callback) {
    OcppServer::getInstance()->setReceiveTXTcallback(ip_addr, callback);
}

#endif
