// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/MessagesV16/StatusNotification.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <ArduinoOcpp/Debug.h>
#include <ArduinoOcpp.h>

#include <string.h>

using ArduinoOcpp::Ocpp16::StatusNotification;

// helper function
namespace ArduinoOcpp
{
    namespace Ocpp16
    {
        const char *cstrFromOcppEveState(OcppEvseState state)
        {
            switch (state)
            {
            case (OcppEvseState::Available):
                ocppsetup_ocpp.setStatus("Available");
                return "Available";
            case (OcppEvseState::Preparing):
                ocppsetup_ocpp.setStatus("Preparing");
                return "Preparing";
            case (OcppEvseState::Charging):
                ocppsetup_ocpp.setStatus("Charging");
                return "Charging";
            case (OcppEvseState::SuspendedEVSE):
                ocppsetup_ocpp.setStatus("SuspendedEVSE");
                return "SuspendedEVSE";
            case (OcppEvseState::SuspendedEV):
                ocppsetup_ocpp.setStatus("SuspendEV");
                return "SuspendedEV";
            case (OcppEvseState::Finishing):
                ocppsetup_ocpp.setStatus("Finishing");
                return "Finishing";
            case (OcppEvseState::Reserved):
                ocppsetup_ocpp.setStatus("Reserved");
                return "Reserved";
            case (OcppEvseState::Unavailable):
                ocppsetup_ocpp.setStatus("Unavailable");
                return "Unavailable";
            case (OcppEvseState::Faulted):
                ocppsetup_ocpp.setStatus("Faulted");
                return "Faulted";
            default:
                AO_DBG_ERR("OcppEvseState not specified");
            case (OcppEvseState::NOT_SET):
                return "NOT_SET";
            }
        }
    }
} // end namespaces

StatusNotification::StatusNotification(int connectorId, OcppEvseState currentStatus, const OcppTimestamp &otimestamp, const char *errorCode)
    : connectorId(connectorId), currentStatus(currentStatus), otimestamp(otimestamp), errorCode(errorCode)
{

    AO_DBG_INFO("New status: %s", cstrFromOcppEveState(currentStatus));
}

const char *StatusNotification::getOcppOperationType()
{
    return "StatusNotification";
}

void StatusNotification::initiate()
{
    // set the most recent EVSE status, but only if it hasn't been specified by the constructor before
    if (currentStatus != OcppEvseState::NOT_SET)
    {
        return;
    }

    if (ocppModel && ocppModel->getChargePointStatusService())  
    {
        auto cpsService = ocppModel->getChargePointStatusService();
        if (connectorId < 0 || connectorId >= cpsService->getNumConnectors())
        {
            if (cpsService->getNumConnectors() == 2)
            {
                // special case: EVSE has exactly 1 physical connector -> take status of the connector
                connectorId = 1;
            }
            else
            {
                // generic EVSE: take status of the whole EVSE
                connectorId = 0;
            }
        }
        auto connector = cpsService->getConnector(connectorId);
        if (connector)
        {
            currentStatus = connector->inferenceStatus();
        }
    }

    if (ocppModel)
    {
        otimestamp = ocppModel->getOcppTime().getOcppTimestampNow();
    }
    else
    {
        otimestamp = MIN_TIME;
    }
    if (currentStatus == OcppEvseState::NOT_SET)
    {
        AO_DBG_ERR("Could not determine EVSE status");
    }
}

// TODO if the status has changed again when sendReq() is called, abort the operation completely (note: if req is already sent, stick with listening to conf). The OcppEvseStateService will enqueue a new operation itself
std::unique_ptr<DynamicJsonDocument> StatusNotification::createReq()
{
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(4) + (JSONDATE_LENGTH + 1)));
    JsonObject payload = doc->to<JsonObject>();

    payload["connectorId"] = connectorId;
    if (errorCode != nullptr)
    {
        payload["errorCode"] = errorCode;
    }
    else if (currentStatus == OcppEvseState::NOT_SET)
    {
        AO_DBG_ERR("Reporting undefined status");
        payload["errorCode"] = "InternalError";
    }
    else
    {
        payload["errorCode"] = "NoError";
    }

    // payload["status"]="Unavailable";
    // currentStatus="Unavailable"
    payload["status"] = cstrFromOcppEveState(currentStatus);

    char timestamp[JSONDATE_LENGTH + 1] = {'\0'};
    otimestamp.toJsonString(timestamp, JSONDATE_LENGTH + 1);
    payload["timestamp"] = timestamp;

    return doc;
}
void StatusNotification::processConf(JsonObject payload)
{
    /*
     * Empty payload
     */
}

StatusNotification::StatusNotification(int connectorId) : connectorId(connectorId)
{
}

/*
 * For debugging only
 */
void StatusNotification::processReq(JsonObject payload)
{
}

/*
 * For debugging only
 */
std::unique_ptr<DynamicJsonDocument> StatusNotification::createConf()
{
    return createEmptyDocument();
}
