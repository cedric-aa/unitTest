#include "vnd_unit_control_aa.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_unit_control_aa, LOG_LEVEL_INF);

static const uint8_t *modeToString[] = {
	[UNIT_CONTROL_MODE_COOL] = "cool mode", [UNIT_CONTROL_MODE_HEAT] = "heat mode",
	[UNIT_CONTROL_MODE_FAN] = "fan mode",	[UNIT_CONTROL_MODE_DRY] = "dry mode",
	[UNIT_CONTROL_MODE_AUTO] = "auto mode",

};

static const uint8_t *fanSpeedToString[] = {
	[UNIT_CONTROL_FAN_SPEEP_1] = "speed 1", [UNIT_CONTROL_FAN_SPEEP_2] = "speed 2",
	[UNIT_CONTROL_FAN_SPEEP_3] = "speed 3", [UNIT_CONTROL_FAN_SPEEP_4] = "speed 4",
	[UNIT_CONTROL_FAN_SPEEP_5] = "speed 5",

};

void printClientStatus(struct btMeshUnitControl *unitControl)
{
	LOG_DBG("The mesh node is provisioned. The client address is 0x%04x.",
		bt_mesh_model_elem(unitControl->model)->addr);
	LOG_DBG("OnOff: [%s] ", unitControl->onOff ? "true" : "false");
	LOG_DBG("Mode: [%s]", modeToString[unitControl->mode]);
	LOG_DBG("Fan Speed: [%s] ", fanSpeedToString[unitControl->fanSpeed]);
	LOG_DBG("Current temperature: [%d,%d-C] ", unitControl->tempValues.currentTemp.integerPart,
		unitControl->tempValues.currentTemp.fractionalPart);
	LOG_DBG("Target temperature: [%d,%d-C] ", unitControl->tempValues.targetTemp.integerPart,
		unitControl->tempValues.targetTemp.fractionalPart);
	LOG_DBG("Unit Control Type: [%d] ", unitControl->unitControlType);
}

void formatUartEncodeFullCmd(dataQueueItemType *uartTxQueueItem, uint8_t *buff, uint8_t len)

{
	memcpy(&uartTxQueueItem->bufferItem[uartTxQueueItem->length], buff, len);
	uartTxQueueItem->bufferItem[0] = uartTxQueueItem->bufferItem[0] + len - 1;
	uartTxQueueItem->length = uartTxQueueItem->length + len;
}
