#include <zephyr/bluetooth/mesh.h>
#include "vnd_unit_control_client_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_unit_control_client_aa, LOG_LEVEL_DBG);

int sendUnitControlFullCmdGet(struct btMeshUnitControl *unitControl, uint16_t addr,
			      uint8_t seqNumber)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = unitControl->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_GET,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_GET);
	net_buf_simple_add_u8(&buf, seqNumber);

	LOG_INF("Send [unitControl][GET] to  0x%04x. sequenceNumber:%d ", ctx.addr, seqNumber);

	return bt_mesh_model_send(unitControl->model, &ctx, &buf, NULL, NULL);
}

int sendUnitControlFullCmdSet(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize,
			      uint16_t address)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = address,
		.app_idx = unitControl->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET);

	for (int i = 0; i < bufSize; ++i) {
		net_buf_simple_add_u8(&msg, buf[i]);
	}

	LOG_INF("Send [unitControl][SET] to 0x%04x. sequenceNumber:%d", ctx.addr, buf[bufSize - 1]);

	return bt_mesh_model_send(unitControl->model, &ctx, &msg, NULL, NULL);
}

static int handleFullCmd(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][STATUS] from 0x%04x rssi:%d sequenceNumber:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshUnitControl *unitControl = model->user_data;
	//	LOG_HEXDUMP_INF(buf->data, buf->len, "net_buf");

	uint8_t buff[buf->len];
	uint8_t len = buf->len;
	memcpy(buff, buf->data, len);

	unitControl->mode = net_buf_simple_pull_u8(buf);
	unitControl->onOff = net_buf_simple_pull_u8(buf);
	unitControl->fanSpeed = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.currentTemp.val1 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.currentTemp.val2 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.targetTemp.val1 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.targetTemp.val2 = net_buf_simple_pull_u8(buf);
	unitControl->unitControlType = net_buf_simple_pull_u8(buf);
	//	uint8_t sequenceNumber = net_buf_simple_pull_u8(buf);
	printClientStatus(unitControl);

	if (unitControl->handlers->fullCmd) {
		unitControl->handlers->fullCmd(ctx, buff, len);
	}

	return 0;
}

static int handleStatusCode(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][STATUS_CODE] from addr:0x%04x. rssi:%d sequenceNumber:%d ",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshUnitControl *unitControl = model->user_data;

	uint8_t status = net_buf_simple_pull_u8(buf);
	uint8_t sequenceNumber = net_buf_simple_pull_u8(buf);

	if (unitControl->handlers->statusCode) {
		unitControl->handlers->statusCode(unitControl, ctx, status, sequenceNumber);
	}

	return 0;
}

const struct bt_mesh_model_op btMeshUnitControlOp[] = {
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE), handleFullCmd },
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET_ACK),
	  handleStatusCode },
	BT_MESH_MODEL_OP_END,
};

static int btMeshUnitControlInit(struct bt_mesh_model *model)
{
	LOG_DBG("btMeshUnitControlInit");
	struct btMeshUnitControl *unitControl = model->user_data;
	unitControl->model = model;
	net_buf_simple_init_with_data(&unitControl->pubMsg, unitControl->buf,
				      sizeof(unitControl->buf));
	unitControl->pub.msg = &unitControl->pubMsg;
	unitControl->pub.update = NULL;

	return 0;
}

const struct bt_mesh_model_cb btMeshUnitControlCb = {
	.init = btMeshUnitControlInit,
};

static void unitControlFullCmd(struct bt_mesh_msg_ctx *ctx, uint8_t *buff, uint8_t len)
{
	// Send to the HUB
	dataQueueItemType uartTxQueueItem =
		headerFormatUartTx(ctx->addr, UNIT_CONTROL_TYPE, STATUS, false);
	formatUartEncodeFullCmd(&uartTxQueueItem, buff, len);

	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}

static void unitControlHandleSatusCode(struct btMeshUnitControl *unitControl,
				       struct bt_mesh_msg_ctx *ctx, uint8_t statusCode,
				       uint8_t sequenceNumber)
{
	// send to the Hub
	dataQueueItemType uartTxQueueItem =
		headerFormatUartTx(ctx->addr, UNIT_CONTROL_TYPE, STATUS_CODE, false);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = statusCode; // status
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = sequenceNumber; // status
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length - 1; // update lenghtpayload
	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}

const struct btMeshUnitControlHandlers unitControlHandlers = {
	.fullCmd = unitControlFullCmd,
	.statusCode = unitControlHandleSatusCode,
};
