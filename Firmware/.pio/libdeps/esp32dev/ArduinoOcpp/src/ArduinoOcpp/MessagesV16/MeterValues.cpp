// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/MessagesV16/MeterValues.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <ArduinoOcpp/Debug.h>
#include <ArduinoOcpp.h> //Matt-x/ArduinoOcpp library
#include <ArduinoOcpp/Tasks/Metering/ConnectorMeterValuesRecorder.h>
#include <ArduinoOcpp/Platform.h>
#include <ArduinoOcpp.h>
#include <ArduinoOcpp/MessagesV16/StopTransaction.h>
#include <ZMPT101B.h>
#include <ACS712.h>

using ArduinoOcpp::Ocpp16::MeterValues;
int t = 1;
int trans = -1;
int lasttransaction = -1;
int transactionId = getTransactionId();
void clear();
int tem2;
int tem;
ZMPT101B voltageSensor(26);
ACS712 sensor(ACS712_05B, 4);

// can only be used for echo server debugging
MeterValues::MeterValues()
{
}
// MeterValues::MeterValues(const std::vector<OcppTimestamp> *sampleTime, const std::vector<float> *energy, const std::vector<float> *power,const std::vector<float> *volt, int connectorId, int transactionId)
    // : connectorId{connectorId}, transactionId{transactionId}
MeterValues::MeterValues(const std::vector<OcppTimestamp> *sampleTime, const std::vector<float> *energy, const std::vector<float> *power, int connectorId, int transactionId)
    : connectorId{connectorId}, transactionId{transactionId}
{
    if (sampleTime)
        this->sampleTime = std::vector<OcppTimestamp>(*sampleTime);
    if (energy)
        this->energy = std::vector<float>(*energy);
    if (power)
        this->power = std::vector<float>(*power);
    // if(volt)
        // this->volt = std::vector<float>(*volt);
}

MeterValues::~MeterValues()
{
}

const char *MeterValues::getOcppOperationType()
{
    return "MeterValues";
}

std::unique_ptr<DynamicJsonDocument> MeterValues::createReq()
{
    // We use 230V because it is the common standard in European countries & Indian states
    // Change to your local, if necessary
    // voltageSensor.calibrate();
    // sensor.calibrate();
    // float I = sensor.getCurrentAC();
    // float *i = &I;
    // std::vector<float>power{*i};
    // float V = voltageSensor.getVoltageAC();
    // float *v = &V;
    // std::vector<float>volt{*v};
    int numEntries = sampleTime.size();

    const size_t VALUE_MAXPRECISION = 10;
    const size_t VALUE_MAXSIZE = VALUE_MAXPRECISION + 7;
    char value_str[VALUE_MAXSIZE] = {'\0'};

    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(
        JSON_OBJECT_SIZE(3)                                          // connectorID, transactionId, meterValue entry
        + JSON_ARRAY_SIZE(numEntries)                                // metervalue array
        + numEntries * JSON_OBJECT_SIZE(1)                           // sampledValue entry
        + numEntries * (JSON_OBJECT_SIZE(1) + (JSONDATE_LENGTH + 1)) // timestamp
        + numEntries * JSON_ARRAY_SIZE(2)                            // sampledValue
        + 2 * numEntries * (JSON_OBJECT_SIZE(1) + VALUE_MAXSIZE)     // value
        + 2 * numEntries * JSON_OBJECT_SIZE(1)                       // measurand
        + 2 * numEntries * JSON_OBJECT_SIZE(1)                       // unit
        + 230));                                                     //"safety space"
    JsonObject payload = doc->to<JsonObject>();

    payload["connectorId"] = connectorId;
    JsonArray meterValues = payload.createNestedArray("meterValue");
    
    for (size_t i = 0; i < sampleTime.size(); i++)
    {
        JsonObject meterValue = meterValues.createNestedObject();
        char timestamp[JSONDATE_LENGTH + 1] = {'\0'};
        OcppTimestamp otimestamp = sampleTime.at(i);
        otimestamp.toJsonString(timestamp, JSONDATE_LENGTH + 1);
        meterValue["timestamp"] = timestamp;
        JsonArray sampledValue = meterValue.createNestedArray("sampledValue");
        // returns wH from temp_value.
        if (energy.size() >= i + 1)
        {
            JsonObject sampledValue_1 = sampledValue.createNestedObject();
            snprintf(value_str, VALUE_MAXSIZE, "%.*g", VALUE_MAXPRECISION, energy.at(i));
            Serial.println("Meter Value");
            Serial.println(value_str);
            char *temp_value = value_str;
            // int temp_value_int = atoi(value_str);
            float temp_value_int = atof(value_str) - meterStop;
            Serial.println(temp_value_int);
            if(temp_value_int >= 0)
            {
                char buffer[1000];
                sprintf(buffer, "%.3f", temp_value_int);
                temp_value = buffer;
                ocppsetup.lcdPrint("               ", 1, 5);
                ocppsetup.lcdPrint("Wh : ", 1, 0);
                ocppsetup.lcdPrint(temp_value, 1, 5);
                ocppsetup.lcdPrint("MTR: ", 3, 0);

                sampledValue_1["value"] = temp_value;
                sampledValue_1["measurand"] = "Energy.Active.Import.Register";
                sampledValue_1["unit"] = "Wh";
            }
            else{
                temp_value_int = temp_value_int * -1;
                char buffer[1000];
                sprintf(buffer, "%.3f", temp_value_int);
                temp_value = buffer;
                ocppsetup.lcdPrint("               ", 1, 5);
                ocppsetup.lcdPrint("Wh : ", 1, 0);
                ocppsetup.lcdPrint(temp_value, 1, 5);
                ocppsetup.lcdPrint("MTR: ", 3, 0);

                sampledValue_1["value"] = temp_value;
                sampledValue_1["measurand"] = "Energy.Active.Import.Register";
                sampledValue_1["unit"] = "Wh";
            }
                    }
        if (power.size() >= i + 1)
        {
            JsonObject sampledValue_1 = sampledValue.createNestedObject();

            snprintf(value_str, VALUE_MAXSIZE, "%.*g", VALUE_MAXPRECISION, energy.at(i));
            Serial.println("Meter Value");
            Serial.println(value_str);
            char *temp_value = value_str;
            // int temp_value_int = atoi(value_str);
            float temp_value_int = atof(value_str) - meterStop;
            Serial.println(temp_value_int);
            if(temp_value_int >= 0)
            {
                char buffer[1000];
                sprintf(buffer, "%.3f", temp_value_int);
                temp_value = buffer;
                ocppsetup.lcdPrint("               ", 1, 5);
                ocppsetup.lcdPrint("W : ", 1, 10);
                ocppsetup.lcdPrint(temp_value, 1, 5);
                ocppsetup.lcdPrint("MTR: ", 3, 0);

                sampledValue_1["value"] = temp_value;
                sampledValue_1["measurand"] = "Energy.Active.Import.Register";
                sampledValue_1["unit"] = "W";
            }
            else{
                temp_value_int = temp_value_int * -1;
                char buffer[1000];
                sprintf(buffer, "%.3f", temp_value_int);
                temp_value = buffer;
                ocppsetup.lcdPrint("               ", 1, 5);
                ocppsetup.lcdPrint("W : ", 1, 10);
                ocppsetup.lcdPrint(temp_value, 1, 5);
                ocppsetup.lcdPrint("MTR: ", 3, 0);

                sampledValue_1["value"] = temp_value;
                sampledValue_1["measurand"] = "Energy.Active.Import.Register";
                sampledValue_1["unit"] = "W";
            }
        }
    }

    if (ocppModel && ocppModel->getConnectorStatus(connectorId))
    {
        auto connector = ocppModel->getConnectorStatus(connectorId);
        if (connector->getTransactionIdSync() >= 0)
        {
            payload["transactionId"] = connector->getTransactionIdSync();
        }
    }


    return doc;
}

void MeterValues::processConf(JsonObject payload)
{
    AO_DBG_DEBUG("Request has been confirmed");
}

void MeterValues::processReq(JsonObject payload)
{

    /**
     * Ignore Contents of this Req-message, because this is for debug purposes only
     */
}

std::unique_ptr<DynamicJsonDocument> MeterValues::createConf()
{
    return createEmptyDocument();
}
