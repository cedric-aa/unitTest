#include <zephyr/bluetooth/mesh.h>
#include "vnd_unit_control_server_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_unit_control_server_aa, LOG_LEVEL_DBG);

static bool statusReceived;

static void sendUnitControlGetToCbUart()
{
	static uint8_t seqNumber;
	LOG_INF("send [UART][UNIT_CONTROL_TYPE][GET] seqNumber %d", seqNumber);
	dataQueueItemType uartTxQueueItem =
		headerFormatUartTx(0x1234, UNIT_CONTROL_TYPE, GET, true);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = seqNumber++;
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length - 1; // update lenghtpayload

	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}

//TODO remove the static address
static void expiryUpdateTimer(struct k_timer *timer_id)
{
	sendUnitControlGetToCbUart();
	if (timer_id->status > 5) {
		statusReceived = false;
		LOG_ERR("unitControl UpdateTimer expired send Message Alert");
		uint8_t buf[2] = { CB_CPU_NOT_RESPONDING, 0x00 };

		dataQueueItemType publisherQueueItem = createPublisherQueueItem(
			false, 0x01ad, UNIT_CONTROL_TYPE, STATUS_CODE, buf, sizeof(buf));

		k_msgq_put(&publisherQueue, &publisherQueueItem, K_NO_WAIT);
		k_timer_stop(&updateTimer);
		k_timer_start(&updateTimer, K_SECONDS(20), K_SECONDS(20));
	}
}

static void expirySetAckTimer(struct k_timer *timer_id)
{
	LOG_INF("unitControl setAck Timer expired");
	uint8_t *data = (uint8_t *)k_timer_user_data_get(&setAckTimer);
	LOG_HEXDUMP_ERR(data, 3, "debug ZC");
	uint16_t addr = ((uint16_t)data[2] << 8) | (uint16_t)data[1];
	uint8_t buf[2] = { SET_TIMEOUT, data[0] };

	dataQueueItemType publisherQueueItem = createPublisherQueueItem(
		false, addr, UNIT_CONTROL_TYPE, STATUS_CODE, buf, sizeof(buf));
	k_msgq_put(&publisherQueue, &publisherQueueItem, K_NO_WAIT);
	k_timer_stop(&setAckTimer);
}

K_TIMER_DEFINE(updateTimer, expiryUpdateTimer, NULL);
K_TIMER_DEFINE(setAckTimer, expirySetAckTimer, NULL);

static void encodeFullCmd(struct net_buf_simple *buf, uint32_t opcode,
			  struct btMeshUnitControl *unitControl, uint8_t seqNumber)
{
	// Initialize the buffer with the given opcode.
	bt_mesh_model_msg_init(buf, opcode);

	// Add all of the parameters to the buffer.
	net_buf_simple_add_u8(buf, unitControl->onOff);
	net_buf_simple_add_u8(buf, unitControl->mode);
	net_buf_simple_add_u8(buf, unitControl->fanSpeed);
	net_buf_simple_add_u8(buf, unitControl->tempValues.currentTemp.integerPart);
	net_buf_simple_add_u8(buf, unitControl->tempValues.currentTemp.fractionalPart);
	net_buf_simple_add_u8(buf, unitControl->tempValues.targetTemp.integerPart);
	net_buf_simple_add_u8(buf, unitControl->tempValues.targetTemp.fractionalPart);
	net_buf_simple_add_u8(buf, unitControl->unitControlType);
	net_buf_simple_add_u8(buf, seqNumber);


}

void sendUnitControlStatusCode(struct btMeshUnitControl *unitControl, uint16_t addr,
			       uint8_t statusCode, uint8_t seqNumber)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = unitControl->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET_ACK);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK);
	net_buf_simple_add_u8(&msg, statusCode);
	net_buf_simple_add_u8(&msg, seqNumber);

	int ret = bt_mesh_model_send(unitControl->model, &ctx, &msg, NULL, NULL);
	if (ret != 0) {
		LOG_ERR("ERROR [%d] send [STATUS_CODE] sequenceNumber:%d", ret, seqNumber);
	} else {
		k_timer_stop(&setAckTimer);
		sendUnitControlGetToCbUart();
		LOG_INF("Send [unitControl][STATUS_CODE][%d] to 0x%04x. sequenceNumber:%d ",
			statusCode, ctx.addr, seqNumber);
	}
}

static int handleFullCmdGet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][GET] from 0x%04x rssi:%d sequenceNumber:%d", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);
	uint8_t seqNumber = net_buf_simple_pull_u8(buf);
	struct btMeshUnitControl *unitControl = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE);
	
	

	if (statusReceived) {

		encodeFullCmd(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE, unitControl, seqNumber);

	

		if (!bt_mesh_model_send(unitControl->model, ctx, &msg, NULL, NULL)) {
			LOG_INF("Send [unitControl][STATUS] to 0x%04x sequenceNumber:%d", ctx->addr,
				seqNumber);

		} else {
			LOG_ERR("Error Send [unitControl][STATUS] to 0x%04x sequenceNumber:%d",
				ctx->addr, seqNumber);
		}
	} else {
		//save the address here
		sendUnitControlStatusCode(unitControl, ctx->addr, WAITING_FOR_RESPONSE, seqNumber);
	}

	return 0;
}

static int handleFullCmdSet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][SET] from 0x%04x rssi:%d sequenceNumber:%d", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshUnitControl *unitControl = model->user_data;
	uint8_t buff[buf->len];
	memcpy(buff, buf->data, buf->len);

	uint8_t dataTimer[3];
	dataTimer[0] = buf->data[buf->len - 1]; // sequence number
	dataTimer[1] = ctx->addr & 0xFF; // low byte of address
	dataTimer[2] = ctx->addr >> 8; // high byte of address

	k_timer_user_data_set(&setAckTimer, &dataTimer);
	k_timer_start(&setAckTimer, K_SECONDS(3), K_FOREVER);

	if (unitControl->handlers->forwardToUart) {
		unitControl->handlers->forwardToUart(false, ctx->addr, UNIT_CONTROL_TYPE, SET, buff,
						     sizeof(buff));
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
	//sendUnitControlGetToCbUart();
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
	//TODO send GetStatus here
	statusReceived = false;

	return 0;
}

static int btMeshUnitControlStart(struct bt_mesh_model *model)
{
	LOG_DBG("btMeshUnitControlStart");
	k_timer_start(&updateTimer, K_SECONDS(0), K_SECONDS(20));
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

const struct btMeshUnitControlHandlers unitControlHandlers = {
	.forwardToUart = forwardToUart,
};

void unitControlUpdateStatus(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize)
{
	statusReceived = true;
	
	unitControl->onOff = buf[0];
	unitControl->mode = buf[1];
	unitControl->fanSpeed = buf[2];
	unitControl->tempValues.currentTemp.integerPart = buf[3];
	unitControl->tempValues.currentTemp.fractionalPart = buf[4];
	unitControl->tempValues.targetTemp.integerPart = buf[5];
	unitControl->tempValues.targetTemp.fractionalPart = buf[6];
	unitControl->unitControlType = buf[7];
	k_timer_stop(&updateTimer);
	k_timer_start(&updateTimer, K_SECONDS(20), K_SECONDS(20));
	printClientStatus(unitControl);
}
