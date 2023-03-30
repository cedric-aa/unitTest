#include <zephyr/bluetooth/mesh.h>
#include "vnd_motor_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_motor, LOG_LEVEL_INF);

int forwardToUart(struct btMeshMotor *motor, struct bt_mesh_msg_ctx *ctx, uint8_t messageId,
		  uint8_t *buf, uint8_t len)
{
	dataQueueItemType uartTxQueueItem =
		headerFormatUartTx(ctx->addr, MOTOR_TYPE, messageId, false);

	for (int i = 0; i < len; i++) {
		uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = buf[i];
	}
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length - 1; // update lenghtpayload
	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);

	return 0;
}
