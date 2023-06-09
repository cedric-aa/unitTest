#include <zephyr/bluetooth/mesh.h>
#include "vnd_motor_server_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_motor_server, LOG_LEVEL_INF);

static bool updatereceived;

static void sendToCbUartMotorStatus()
{
	static uint8_t seqNumber;
	LOG_INF("send [UART][MOTOR_TYPE][GET] seqNumber %d", seqNumber);
	dataQueueItemType uartTxQueueItem = headerFormatUartTx(0x01ad, MOTOR_TYPE, GET, true);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = seqNumber++;
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length - 1; // update lenghtpayload

	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}

static void expiryMotorUpdateTimer(struct k_timer *timer_id)
{
	sendToCbUartMotorStatus();
	if (timer_id->status > 5) {
		LOG_ERR("timer_id->status > 5 send Message Alert");
		uint8_t buf[2] = { CB_CPU_NOT_RESPONDING, 0x00 };
		dataQueueItemType publisherQueueItem = createPublisherQueueItem(
			false, 0x01ad, MOTOR_TYPE, STATUS_CODE, buf, sizeof(buf));
		k_msgq_put(&publisherQueue, &publisherQueueItem, K_NO_WAIT);
		k_timer_stop(&motorUpdateTimer);
		k_timer_start(&motorUpdateTimer, K_SECONDS(20), K_SECONDS(20));
	}
}

static void expiryMotorSetAckTimer(struct k_timer *timer_id)
{
	LOG_DBG("SetAct Timeout");
	uint8_t *data = (uint8_t *)k_timer_user_data_get(&motorSetAckTimer);


	uint16_t addr = ((uint16_t)data[2] << 8) | (uint16_t)data[1];
	uint8_t buf[2] = { SET_TIMEOUT, data[0] };

	dataQueueItemType publisherQueueItem =
		createPublisherQueueItem(false, addr, MOTOR_TYPE, STATUS_CODE, buf, sizeof(buf));
	k_msgq_put(&publisherQueue, &publisherQueueItem, K_NO_WAIT);
	k_timer_stop(&motorSetAckTimer);
}

K_TIMER_DEFINE(motorUpdateTimer, expiryMotorUpdateTimer, NULL);
K_TIMER_DEFINE(motorSetAckTimer, expiryMotorSetAckTimer, NULL);

void motorUpdateStatus(struct btMeshMotor *motor, uint8_t *buf, size_t bufSize)
{
	updatereceived = true;
	k_timer_stop(&motorUpdateTimer);
	k_timer_start(&motorUpdateTimer, K_SECONDS(20), K_SECONDS(20));

	for (int i = 0; i < bufSize; i++) {
		motor->motorLevel[i] = buf[i];
	}
}

void motorUpdateStatusId(struct btMeshMotor *motor, uint8_t id, uint8_t level)
{
	motor->motorLevel[id] = level;
}

int sendMotorStatusCode(struct btMeshMotor *motor, uint16_t addr, uint8_t statusCode,
			uint8_t seqNum)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = motor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_MOTOR_OP_STATUS_CODE,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_CODE);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_MOTOR_OP_STATUS_CODE);
	net_buf_simple_add_u8(&msg, statusCode);
	net_buf_simple_add_u8(&msg, seqNum);

	int ret = bt_mesh_model_send(motor->model, &ctx, &msg, NULL, NULL);

	if (!ret) {
		LOG_INF("Send [MOTOR_TYPE][STATUS_CODE] to 0x%04x seqNum:%d ", ctx.addr, seqNum);
		k_timer_stop(&motorSetAckTimer);
		sendToCbUartMotorStatus();
	} else {
		LOG_ERR("ERROR Send [MOTOR_TYPE][STATUS_CODE] to 0x%04x seqNum:%d", ctx.addr,
			seqNum);
	}
	return ret;
}

static int sendStatusId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t id,
			uint8_t seqNum)
{
	struct btMeshMotor *motor = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ID,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ID);
	net_buf_simple_add_u8(&msg, id);
	net_buf_simple_add_u8(&msg, motor->motorLevel[id]);
	net_buf_simple_add_u8(&msg, seqNum);

	int ret = bt_mesh_model_send(motor->model, ctx, &msg, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [MOTOR_TYPE][STATUS_ID] to 0x%04x. seqNum:%d ", ctx->addr, seqNum);

	} else {
		LOG_ERR("ERROR Send [MOTOR_TYPE][STATUS_ID] to 0x%04x. seqNum:%d", ctx->addr,
			seqNum);
	}
	return ret;
}

static int sendStatusAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t seqNum)
{
	struct btMeshMotor *motor = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ALL,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ALL);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ALL);

	for (int i = 0; i < sizeof(motor->motorLevel); i++) {
		net_buf_simple_add_u8(&msg, motor->motorLevel[i]);
	}
	net_buf_simple_add_u8(&msg, seqNum);

	int ret = bt_mesh_model_send(motor->model, ctx, &msg, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [MOTOR_TYPE][STATUS_ALL] to 0x%04x. seqNum:%d ", ctx->addr, seqNum);

	} else {
		LOG_ERR("ERROR[%d] Send [MOTOR_TYPE][STATUS_ALL] to 0x%04x. seqNum:%d ", ret,
			ctx->addr, seqNum);
	}
	return ret;
}

static int handleSet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	LOG_INF("Received [MOTOR_TYPE][SET] from addr:0x%04x. rssi:%d seqNumber:%d", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshMotor *motor = model->user_data;
	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);

	uint8_t dataTimer[3];
	dataTimer[0] = buf->data[buf->len - 1]; // sequence number
	dataTimer[1] = ctx->addr & 0xFF; // low byte of address
	dataTimer[2] = ctx->addr >> 8; // high byte of address

	k_timer_user_data_set(&motorSetAckTimer, &dataTimer);
	k_timer_start(&motorSetAckTimer, K_SECONDS(4), K_FOREVER);

	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(false, ctx->addr, MOTOR_TYPE, SET, data,
					       sizeof(data));
	}

	return 0;
}

static int handleGetId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	LOG_INF("Received [MOTOR_TYPE][GET_ID] from addr:0x%04x rssi:%d tid:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);
	uint8_t motorId = net_buf_simple_pull_u8(buf);
	uint8_t seqNumber = net_buf_simple_pull_u8(buf);

	if (updatereceived) {
		sendStatusId(model, ctx, motorId, seqNumber);
	} else {
		//wait for the cb to upddate the datamodel
		sendMotorStatusCode(model->user_data, ctx->addr, WAITING_FOR_RESPONSE,
				    buf->data[buf->len - 1]);
	}
	return 0;
}

static int handleGetAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	LOG_INF("Received [MOTOR_TYPE][GET_ALL] from addr:0x%04x rssi:%d tid:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	uint8_t seqNumber = net_buf_simple_pull_u8(buf);
	if (updatereceived) {
		sendStatusAll(model, ctx, seqNumber);
	} else {
		sendMotorStatusCode(model->user_data, ctx->addr, WAITING_FOR_RESPONSE,
				    buf->data[buf->len - 1]);
	}
	return 0;
}

const struct bt_mesh_model_op btMeshMotorOp[] = {
	{ BT_MESH_MODEL_MOTOR_OP_SET, BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_SET),
	  handleSet },
	{ BT_MESH_MODEL_MOTOR_OP_GET_ID,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET_ID), handleGetId },
	{ BT_MESH_MODEL_MOTOR_OP_GET_ALL,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET_ALL), handleGetAll },
	BT_MESH_MODEL_OP_END,
};

static int btMeshMotorUpdateHandler(struct bt_mesh_model *model)
{
	LOG_DBG("MotorUpdateHandler");
	sendToCbUartMotorStatus();
	return 0;
}

static int btMeshMotorInit(struct bt_mesh_model *model)
{
	struct btMeshMotor *motor = model->user_data;
	motor->model = model;
	net_buf_simple_init_with_data(&motor->pubMsg, motor->buf, sizeof(motor->buf));
	motor->pub.msg = &motor->pubMsg;
	motor->pub.update = btMeshMotorUpdateHandler;
	updatereceived = false;
	return 0;
}

static int btMeshMotorStart(struct bt_mesh_model *model)
{
	LOG_DBG("Motor start");
	k_timer_start(&motorUpdateTimer, K_SECONDS(1), K_SECONDS(20));
	return 0;
}

static void btMeshMotorReset(struct bt_mesh_model *model)
{
	LOG_DBG("Reset motor model");
}

const struct bt_mesh_model_cb btMeshMotorCb = {
	.init = btMeshMotorInit,
	.start = btMeshMotorStart,
	.reset = btMeshMotorReset,
};

const struct btMeshMotorHandlers motorHandlers = {
	.forwardToUart = forwardToUart,
};
