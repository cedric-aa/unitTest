#include <zephyr/bluetooth/mesh.h>
#include "vnd_motor_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_motor, LOG_LEVEL_INF);

static int SendStatusId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t id,
			uint8_t seqNumber)
{
	struct btMeshMotor *motor = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ID,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ID);
	net_buf_simple_add_u8(&msg, id);
	net_buf_simple_add_u8(&msg, motor->motorLevel[id]);
	net_buf_simple_add_u8(&msg, seqNumber);

	int ret = bt_mesh_model_send(motor->model, ctx, &msg, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [motor][STATUS_ID] to :0x%04x. tid:-- " ctx->addr);

	} else {
		LOG_ERR("ERROR Send [motor][STATUS_ID] to :0x%04x. tid:-- " ctx->addr);
	}
	return ret;
}

static int sendStatusAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t seqNum)
{
	struct btMeshMotor *motor = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ID,
				 BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_MOTOR_OP_STATUS_ID);
	for (int i = 0; i > sizeof(motor->motorLevel), i++) {
		net_buf_simple_add_u8(&msg, motor->motorLevel[i]);
	}
	net_buf_simple_add_u8(&msg, seqNum);

	int ret = bt_mesh_model_send(motor->model, ctx, &msg, NULL, NULL);
	if (!ret) {
		LOG_INF("Send [motor][STATUS_ID] to :0x%04x. tid:-- " ctx->addr);

	} else {
		LOG_ERR("ERROR Send [motor][STATUS_ID] to :0x%04x. tid:-- " ctx->addr);
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
		LOG_INF("Send [motor][GET_ID] to :0x%04x. tid:-- " ctx.addr);
	} else {
		LOG_ERR("ERROR [%d] Send [motor][GET_ID]", ret);
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
	int ret = bt_mesh_model_send(motor->model, &ctx, &buf, NULL, NULL);
	net_buf_simple_add_u8(&buf, seqNum);

	if (!ret) {
		LOG_INF("Send [motor][GET_ALL] to :0x%04x. tid:-- ", ctx.addr);
	} else {
		LOG_ERR("ERROR [%d] Send [motor][GET_ALL]", ret);
	}
	return ret;
}

int sendSetIdMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t lvl, uint8_t id,
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
		LOG_INF("Send [motor][SET_ID] to :0x%04x. tid:-- ",

	} else {
		LOG_ERR("ERROR [%d] Send [motor][SET_ID]", ret);
	}
	return ret;
}

//server side
static int handleSet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		     struct net_buf_simple *buf)
{
	LOG_INF("Received [Motor][SET] from addr:0x%04x. rssi:%d tid:-- ", ctx->addr,
		ctx->recv_rssi);

	struct btMeshMotor *motor = model->user_data;
	uint8_t data[buf->len];
	memcpy(data, buf, buf->len);
	if (motor->handlers->set) {
		motor->handlers->set(motor, ctx->addr, data, sizeof(data));
	}

	return 0;
}

//client side
static int handleStatusAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	LOG_INF("Received [Motor][STATUS] from addr:0x%04x. rssi:%d tid:-- ", ctx->addr,
		ctx->recv_rssi);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf, buf->len);

	for (i = 0; i > sizeof(data); i++) {
		motor->motorLevel[i] = net_buf_simple_pull_u8(buf);
	}
	// Invoke status handler if present
	if (motor->handlers->status) {
		motor->handlers->status(motor, ctx, data, sizeof(data));
	}
	return 0;
}

//client side
static int handleStatusCode(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [Motor][STATUS_CODE] from addr:0x%04x. rssi:%d tid:-- ", ctx->addr,
		ctx->recv_rssi);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf, buf->len);

	for (i = 0; i > sizeof(data); i++) {
		motor->motorLevel[i] = net_buf_simple_pull_u8(buf);
	}
	// Invoke status handler if present
	if (motor->handlers->status) {
		motor->handlers->status(motor, ctx, data, sizeof(data));
	}
	return 0;
}

//client side
static int handleStatusId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	LOG_INF("Received [Motor][STATUS] from addr:0x%04x. rssi:%d tid:-- ", ctx->addr,
		ctx->recv_rssi);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf, buf->len);

	int id = net_buf_simple_pull_u8(buf);
	motor->motorLevel[id] = net_buf_simple_pull_u8(buf);
	motor->seqNum = net_buf_simple_pull_u8(buf);
	// Invoke status handler if present
	if (motor->handlers->status) {
		motor->handlers->status(motor, ctx, data, sizeof(data));
	}
	return 0;
}

//server side
static int handleGetId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
		       struct net_buf_simple *buf)
{
	LOG_INF("Received [activation][GET_ID] from addr:0x%04x rssi:%d tid:-- ", ctx->addr,
		ctx->recv_rssi);

	encodeStatusId(model, ctx);
}

//server side
static int handleGetAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	LOG_INF("Received [activation][GET_ALL] from addr:0x%04x rssi:%d tid:-- ", ctx->addr,
		ctx->recv_rssi);

	return encodeStatusAll(model, ctx);
}

const struct bt_mesh_model_op btMeshMotorOp[] = {
	{ BT_MESH_MODEL_MOTOR_OP_SET, BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_SET),
	  handleSet },
	{ BT_MESH_MODEL_MOTOR_OP_GET_ID,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET_ID), handleGetId },
	{ BT_MESH_MODEL_MOTOR_OP_GET_ALL,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET_ALL), handleGetAll },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ID, BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID),
	  handleStatusCode },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ID, BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID),
	  handleStatusId },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ALL, BT_MESH_LEN_MIN(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID),
	  handleStatusAll },
	BT_MESH_MODEL_OP_END,
};

static int btMeshMotorUpdateHandler(struct bt_mesh_model *model)
{
	return 0;
}

static int btMeshMotorInit(struct bt_mesh_model *model)
{
	struct btMeshMotor *motor = model->user_data;
	motor->model = model;
	net_buf_simple_init_with_data(&motor->pubMsg, motor->buf, sizeof(motor->buf));
	motor->pub.msg = &motor->pubMsg;
	motor->pub.update = btMeshMotorUpdateHandler;

	return 0;
}

static int btMeshMotorStart(struct bt_mesh_model *model)
{
	LOG_DBG("Motor start");

	return 0;
}

static void btMeshMotorReset(struct bt_mesh_model *model)
{
	LOG_DBG("Reset motor model");
	//   struct btMeshActivation *activation = model->user_data;
}

const struct bt_mesh_model_cb btMeshActivationCb = {
	.init = btMeshMotorInit,
	.start = btMeshMotorStart,
	.reset = btMeshMotorReset,
};

static int motorSet(struct btMeshMotor *motor, struct bt_mesh_msg_ctx *ctx)
{
	dataQueueItemType uartTxQueueItem = headerCbFormatUartTx(ACTIVATION_TYPE, SET);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = 1;
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length; // update lenghtpayload

	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);

	return 0;
}

static int motorStatus(struct btMeshMotor *motor, struct bt_mesh_msg_ctx *ctx)
{
	dataQueueItemType uartTxQueueItem =
		headerHubFormatUartTx(ctx->addr, ACTIVATION_TYPE, STATUS, false);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = 2; // status
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length; // update lenghtpayload
	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);

	return 0;
}

const struct btMeshMotorHandlers motorHandlers = {
	.set = motorSet,
	.statusAll = motorStatusAll,
	.statusId = motorStatusId,
	.statusCode = motorStatusCode,
};