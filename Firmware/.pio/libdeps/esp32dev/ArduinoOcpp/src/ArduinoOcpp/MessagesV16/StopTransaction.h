// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#ifndef STOPTRANSACTION_H
#define STOPTRANSACTION_H

#include <ArduinoOcpp/Core/OcppMessage.h>
#include <ArduinoOcpp/Core/OcppTime.h>
extern int meterStop ;
extern int meterStop1;
namespace ArduinoOcpp {
namespace Ocpp16 {

class StopTransaction : public OcppMessage {
private:
    int connectorId = 1;
    // int meterStop = -1;
    OcppTimestamp otimestamp;
public:
    // int meterStop = 0;
    // int meterStop1;
    StopTransaction(int connectorId);

    const char* getOcppOperationType();

    void initiate();

    std::unique_ptr<DynamicJsonDocument> createReq();

    void processConf(JsonObject payload);

    void processReq(JsonObject payload);

    std::unique_ptr<DynamicJsonDocument> createConf();
};

} //end namespace Ocpp16
} //end namespace ArduinoOcpp
#endif
