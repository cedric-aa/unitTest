#include <zephyr/bluetooth/mesh.h>
#include "vnd_unit_control_server_aa.h"

#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_unit_control_server_aa, LOG_LEVEL_DBG);

static void expiryupdateTimer(struct k_timer *timer_id)
{
	LOG_DBG("expiryupdateTimer");
	sendToCbUartStatus();
	if (timer_id->status > 5) {
		LOG_ERR("timer_id->status > 5 send Message Alert");
		//send alert to the hub
		sendUnitControlStatusCode(&unitControl, 5);
		//k_timer_stop(&updateTimer);
	}
}

static void expirysetAckTimer(struct k_timer *timer_id)
{
	LOG_DBG("//setAcktimeout");
	//setAcktimeout
	sendUnitControlStatusCode(&unitControl, 1);
}

K_TIMER_DEFINE(updateTimer, expiryupdateTimer, NULL);
K_TIMER_DEFINE(setAckTimer, expirysetAckTimer, NULL);

static void encodeFullCmd(struct net_buf_simple *buf, uint32_t opcode,
			  struct btMeshUnitControl *unitControl)
{
	// Initialize the buffer with the given opcode.
	bt_mesh_model_msg_init(buf, opcode);

	// Add all of the parameters to the buffer.
	net_buf_simple_add_u8(buf, unitControl->mode);
	net_buf_simple_add_u8(buf, unitControl->onOff);
	net_buf_simple_add_u8(buf, unitControl->fanSpeed);
	net_buf_simple_add_u8(buf, unitControl->tempValues.currentTemp.val1);
	net_buf_simple_add_u8(buf, unitControl->tempValues.currentTemp.val2);
	net_buf_simple_add_u8(buf, unitControl->tempValues.targetTemp.val1);
	net_buf_simple_add_u8(buf, unitControl->tempValues.targetTemp.val2);
	net_buf_simple_add_u8(buf, unitControl->unitControlType);
}

void sendUnitControlStatusCode(struct btMeshUnitControl *unitControl, uint8_t statusCode)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = 0xC001, //group address
		.app_idx = unitControl->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET_ACK);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK);

	net_buf_simple_add_u8(&msg, statusCode);

	int ret = bt_mesh_model_send(unitControl->model, &ctx, &msg, NULL, NULL);
	if (ret != 0) {
		LOG_ERR("ERROR [%d] send [STATUS_CODE] [%d]", ret, statusCode);
	} else {
		k_timer_stop(&setAckTimer);
		LOG_INF("Send [unitControl][STATUS_CODE][%d] to :0x%04x. tid:-- ", statusCode,
			ctx.addr);
	}
}

static int handleFullCmdGet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received Mesh [unitControl][GET] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi);
	struct btMeshUnitControl *unitControl = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE);

	bool statusReceived = true;

	if (statusReceived) {
		encodeFullCmd(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE, unitControl);
		LOG_INF("Send [unitControl][STATUS] op[0x%06x] from addr:0x%04x. revc_addr:0x%04x. tid:-- ",
			BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE, ctx->recv_dst, ctx->addr);
		(void)bt_mesh_model_send(unitControl->model, ctx, &msg, NULL, NULL);
	} else {
		sendUnitControlStatusCode(unitControl, 5);
	}

	return 0;
}

static int handleFullCmdSet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][SET] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi);

	struct btMeshUnitControl *unitControl = model->user_data;
	uint8_t buff[buf->len];
	memcpy(buff, buf->data, buf->len);
	if (unitControl->handlers->fullCmdSet) {
		unitControl->handlers->fullCmdSet(buff, buf->len);
	}

	return 0;
}

const struct bt_mesh_model_op btMeshUnitControlOp[] = {
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET),
	  handleFullCmdSet },
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_GET,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_GET),
	  handleFullCmdGet },
	BT_MESH_MODEL_OP_END,
};

static int btMeshUnitControlUpdateHandler(struct bt_mesh_model *model)
{
	LOG_DBG("UpdateHandler");
	sendToCbUartStatus();
	return 0;
}

static int btMeshUnitControlInit(struct bt_mesh_model *model)
{
	LOG_DBG("btMeshUnitControlInit");
	struct btMeshUnitControl *unitControl = model->user_data;
	unitControl->model = model;
	net_buf_simple_init_with_data(&unitControl->pubMsg, unitControl->buf,
				      sizeof(unitControl->buf));
	unitControl->pub.msg = &unitControl->pubMsg;
	unitControl->pub.update = btMeshUnitControlUpdateHandler;

	unitControl->mode = 1;
	unitControl->onOff = 1;
	unitControl->fanSpeed = 1;
	unitControl->tempValues.currentTemp.val1 = 1;
	unitControl->tempValues.currentTemp.val2 = 1;
	unitControl->tempValues.targetTemp.val1 = 1;
	unitControl->tempValues.targetTemp.val2 = 1;
	unitControl->unitControlType = 1;
	return 0;
}

static int btMeshUnitControlStart(struct bt_mesh_model *model)
{
	LOG_DBG("btMeshUnitControlStart");
	k_timer_start(&updateTimer, K_SECONDS(1), K_SECONDS(10));
	return 0;
}

static void btMeshUnitControlReset(struct bt_mesh_model *model)
{
	LOG_DBG("btMeshUnitControlReset");
}

const struct bt_mesh_model_cb btMeshUnitControlCb = {
	.init = btMeshUnitControlInit,
	.start = btMeshUnitControlStart,
	.reset = btMeshUnitControlReset,
};

static void unitControlFullCmdSet(uint8_t *buff, uint8_t len)
{
	// send to the CB
	dataQueueItemType uartTxQueueItem = headerCbFormatUartTx(UNIT_CONTROL_TYPE, SET);
	formatUartEncodeFullCmd(&uartTxQueueItem, buff, len);
	int ret = k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
	if (!ret) {
		k_timer_start(&setAckTimer, K_SECONDS(10), K_FOREVER);
	}
}

const struct btMeshUnitControlHandlers unitControlHandlers = {
	.fullCmdSet = unitControlFullCmdSet,
};

void unitControlUpdateStatus(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize)
{
	unitControl->mode = buf[2];
	unitControl->onOff = buf[3];
	unitControl->fanSpeed = buf[4];
	unitControl->tempValues.currentTemp.val1 = buf[5];
	unitControl->tempValues.currentTemp.val2 = buf[6];
	unitControl->tempValues.targetTemp.val1 = buf[7];
	unitControl->tempValues.targetTemp.val2 = buf[8];
	unitControl->unitControlType = buf[9];
	printClientStatus(unitControl);
}

void sendToCbUartStatus()
{
	LOG_INF("send uart [UNIT_CONTROL_TYPE][GET] to the Cb");
	dataQueueItemType uartTxQueueItem = headerCbFormatUartTx(UNIT_CONTROL_TYPE, GET);
	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}