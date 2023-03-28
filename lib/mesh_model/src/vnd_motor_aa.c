#include <zephyr/bluetooth/mesh.h>
#include "vnd_motor_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_motor, LOG_LEVEL_INF);

static void sendToCbUartMotorStatus()
{
	static uint8_t seqNumber;
	LOG_INF("send [UART][UNIT_CONTROL_TYPE][GET] seqNumber %d", seqNumber);
	dataQueueItemType uartTxQueueItem = headerFormatUartTx(0x0101, MOTOR_TYPE, GET, true);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = seqNumber++;
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length; // update lenghtpayload

	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}

static void expiryMotorUpdateTimer(struct k_timer *timer_id)
{
	LOG_DBG("expirymotorUpdateTimer");
	sendToCbUartMotorStatus();
	if (timer_id->status > 5) {
		LOG_ERR("timer_id->status > 5 send Message Alert");
		//send alert to the hub
		//send to a groupAdress
		sendMotorStatusCode(&motor, 0x0198, 0x03, 0);
		k_timer_stop(&motorUpdateTimer);
	}
}

static void expiryMotorSetAckTimer(struct k_timer *timer_id)
{
	LOG_DBG("//setAcktimeout");
	//setAcktimeout
	//find the sequence number
	uint8_t *seq = k_timer_user_data_get(&motorSetAckTimer);
	LOG_INF(" test datasaved must be 13 = [%d]", *seq);
	sendMotorStatusCode(&motor, 0x0196, 0x01, *seq);
}

K_TIMER_DEFINE(motorUpdateTimer, expiryMotorUpdateTimer, NULL);
K_TIMER_DEFINE(motorSetAckTimer, expiryMotorSetAckTimer, NULL);

void motorUpdateStatus(struct btMeshMotor *motor, uint8_t *buf, size_t bufSize)
{
	for (int i = 0; i > bufSize; i++) {
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
		LOG_INF("Send [MESH][MOTOR_TYPE][STATUS_CODE] to 0x%04x seqNum:%d ", ctx.addr, seqNum);
		k_timer_stop(&motorSetAckTimer);
	} else {
		LOG_INF("ERROR Send [MESH][MOTOR_TYPE][STATUS_CODE] to 0x%04x seqNum:%d", ctx.addr, seqNum);
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
		LOG_INF("Send [MESH][MOTOR_TYPE][STATUS_ID] to 0x%04x. seqNum:%d ", ctx->addr, seqNum);

	} else {
		LOG_ERR("ERROR Send [MESH][MOTOR_TYPE][STATUS_ID] to 0x%04x. seqNum:%d", ctx->addr, seqNum);
	}
	return ret;
}

static int sendStatusAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t seqNum)
{
	struct btMeshMotor *motor = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ALL,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ALL);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ALL);

	for (int i = 0; i > sizeof(motor->motorLevel); i++) {
		net_buf_simple_add_u8(&msg, motor->motorLevel[i]);
	}
	net_buf_simple_add_u8(&msg, seqNum);

	int ret = bt_mesh_model_send(motor->model, ctx, &msg, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [MESH][MOTOR_TYPE][STATUS_ID] to 0x%04x. seqNum:%d ", ctx->addr, seqNum);

	} else {
		LOG_ERR("ERROR[%d] Send [MESH][MOTOR_TYPE][STATUS_ID] to 0x%04x. seqNum:%d ", ret, ctx->addr,
			seqNum);
	}
	return ret;
}

int sendGetIdMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t id, uint8_t seqNum)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = motor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_MOTOR_OP_GET_ID,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET_ID);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_MOTOR_OP_GET_ID);
	net_buf_simple_add_u8(&buf, id);
	net_buf_simple_add_u8(&buf, seqNum);

	int ret = bt_mesh_model_send(motor->model, &ctx, &buf, NULL, NULL);

	if (!ret) {
		LOG_INF("Send [MESH][MOTOR_TYPE][GET_ID] to 0x%04x. seqNum:%d ", ctx.addr, seqNum);
	} else {
		LOG_ERR("ERROR [%d] Send [MESH][MOTOR_TYPE][GET_ID] to 0x%04x. seqNum:%d ", ret, ctx.addr,
			seqNum);
	}
	return ret;
}

int sendGetAllMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t seqNum)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = motor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_MOTOR_OP_GET_ALL,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET_ALL);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_MOTOR_OP_GET_ALL);
	net_buf_simple_add_u8(&buf, seqNum);
	int ret = bt_mesh_model_send(motor->model, &ctx, &buf, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [MESH][MOTOR_TYPE][GET_ALL] to 0x%04x. seqNum:%d ", ctx.addr, seqNum);
	} else {
		LOG_ERR("ERROR [%d] Send [MESH][MOTOR_TYPE][GET_ALL]  to 0x%04x. seqNum:%d ", ret, ctx.addr,
			seqNum);
	}
	return ret;
}

int sendSetIdMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t id, uint8_t lvl,
			uint8_t seqNum)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = motor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_MOTOR_OP_SET, BT_MESH_MODEL_MOTOR_OP_LEN_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_MOTOR_OP_SET);
	net_buf_simple_add_u8(&buf, id); // id
	net_buf_simple_add_u8(&buf, lvl); // level
	net_buf_simple_add_u8(&buf, seqNum); // seqNum

	int ret = bt_mesh_model_send(motor->model, &ctx, &buf, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [MESH][MOTOR_TYPE][SET_ID] to 0x%04x. seqNum:%d ", ctx.addr, seqNum);

	} else {
		LOG_ERR("ERROR [%d] Send [MESH][MOTOR_TYPE][SET_ID] to 0x%04x. seqNum:%d ", ret, ctx.addr,
			seqNum);
	}
	return ret;
}

//server side
static int handleSet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][SET] from addr:0x%04x. rssi:%d seqNumber:%d", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshMotor *motor = model->user_data;
	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);
	//	uint8_t sequenceNumber = buff[len-1];
	static uint8_t sequenceNumber = 13;

	k_timer_user_data_set(&motorSetAckTimer, &sequenceNumber);
	k_timer_start(&motorSetAckTimer, K_SECONDS(10), K_FOREVER);

	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(motor, ctx, SET, data, sizeof(data));
	}

	return 0;
}

//client side
static int handleStatusAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][STATUS] from addr:0x%04x. rssi:%d tid:%d", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);

	for (int i = 0; i > sizeof(data); i++) {
		motor->motorLevel[i] = net_buf_simple_pull_u8(buf);
	}
	// Invoke status handler if present
	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(motor, ctx, STATUS, data, sizeof(data));
	}
	return 0;
}

//client side
static int handleStatusCode(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][STATUS_CODE] from addr:0x%04x. rssi:%d tid:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);

	// Invoke status handler if present
	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(motor, ctx, STATUS_CODE, data, sizeof(data));
	}
	return 0;
}

//client side
static int handleStatusId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][STATUS_ID] from addr:0x%04x. rssi:%d tid:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);

	int id = net_buf_simple_pull_u8(buf);
	motor->motorLevel[id] = net_buf_simple_pull_u8(buf);
	motor->seqNumber = net_buf_simple_pull_u8(buf);
	// Invoke status handler if present
	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(motor, ctx, STATUS_ID, data, sizeof(data));
	}
	return 0;
}

//server side
static int handleGetId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][GET_ID] from addr:0x%04x rssi:%d tid:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);
	bool updatereceived = true;
	uint8_t motorId = net_buf_simple_pull_u8(buf);
	uint8_t seqNumber = net_buf_simple_pull_u8(buf);

	if (updatereceived) {
		sendStatusId(model, ctx, motorId, seqNumber);
	} else {
		//wait for the cb to upddate the datamodel
		sendMotorStatusCode(model->user_data, ctx->addr, 0x10, buf->data[buf->len - 1]);
	}
	return 0;
}

//server side
static int handleGetAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	bool updatereceived = true;
	uint8_t seqNumber = net_buf_simple_pull_u8(buf);
	if (updatereceived) {
		sendStatusAll(model, ctx, seqNumber);
	} else {
		sendMotorStatusCode(model->user_data, ctx->addr, 0x15, buf->data[buf->len - 1]);
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
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_CODE,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_CODE), handleStatusCode },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ID, BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID),
	  handleStatusId },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ALL, BT_MESH_LEN_MIN(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ALL),
	  handleStatusAll },
	BT_MESH_MODEL_OP_END,
};
//todo
static int btMeshMotorUpdateHandler(struct bt_mesh_model *model)
{
	LOG_DBG("MotorUpdateHandler");
	sendToCbUartMotorStatus();
	return 0;
}
//todo
static int btMeshMotorInit(struct bt_mesh_model *model)
{
	struct btMeshMotor *motor = model->user_data;
	motor->model = model;
	net_buf_simple_init_with_data(&motor->pubMsg, motor->buf, sizeof(motor->buf));
	motor->pub.msg = &motor->pubMsg;
	motor->pub.update = btMeshMotorUpdateHandler;

	return 0;
}
//todo
static int btMeshMotorStart(struct bt_mesh_model *model)
{
	LOG_DBG("Motor start");
	k_timer_start(&motorUpdateTimer, K_SECONDS(1), K_SECONDS(10));
	return 0;
}
//todo
static void btMeshMotorReset(struct bt_mesh_model *model)
{
	LOG_DBG("Reset motor model");
}
//todo
const struct bt_mesh_model_cb btMeshMotorCb = {
	.init = btMeshMotorInit,
	.start = btMeshMotorStart,
	.reset = btMeshMotorReset,
};

static int forwardToUart(struct btMeshMotor *motor, struct bt_mesh_msg_ctx *ctx, uint8_t messageId,
			 uint8_t *buf, uint8_t len)
{
	dataQueueItemType uartTxQueueItem =
		headerFormatUartTx(ctx->addr, MOTOR_TYPE, STATUS, false);

	for (int i = 0; i > len; i++) {
		uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = buf[i];
	}
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length; // update lenghtpayload
	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);

	return 0;
}

const struct btMeshMotorHandlers motorHandlers = {
	.forwardToUart = forwardToUart,
};
