#include <zephyr/bluetooth/mesh.h>
#include "vnd_motor_client_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_motor_client, LOG_LEVEL_INF);

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
		LOG_ERR("ERROR [%d] Send [MESH][MOTOR_TYPE][GET_ID] to 0x%04x. seqNum:%d ", ret,
			ctx.addr, seqNum);
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
		LOG_ERR("ERROR [%d] Send [MESH][MOTOR_TYPE][GET_ALL]  to 0x%04x. seqNum:%d ", ret,
			ctx.addr, seqNum);
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
		LOG_ERR("ERROR [%d] Send [MESH][MOTOR_TYPE][SET_ID] to 0x%04x. seqNum:%d ", ret,
			ctx.addr, seqNum);
	}
	return ret;
}

//client side
static int handleStatusAll(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][STATUS] from addr:0x%04x. rssi:%d seqNumber:%d",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);
	//LOG_HEXDUMP_INF(msg.data, msg.len, "buf net");

	for (int i = 0; i < sizeof(data); i++) {
		motor->motorLevel[i] = net_buf_simple_pull_u8(buf);
	}
	// Invoke status handler if present
	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(false, ctx->addr, MOTOR_TYPE, STATUS, data,
					       sizeof(data));
	}
	return 0;
}

//client side
static int handleStatusCode(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][STATUS_CODE] from addr:0x%04x. rssi:%d sequenceNumber:%d ",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);

	// Invoke status handler if present
	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(false, ctx->addr, MOTOR_TYPE, STATUS_CODE, data,
					       sizeof(data));
	}
	return 0;
}

//client side
static int handleStatusId(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	LOG_INF("Received [MESH][MOTOR_TYPE][STATUS_ID] from addr:0x%04x. rssi:%d sequenceNumber:%d ",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);
	struct btMeshMotor *motor = model->user_data;

	uint8_t data[buf->len];
	memcpy(data, buf->data, buf->len);

	int id = net_buf_simple_pull_u8(buf);
	motor->motorLevel[id] = net_buf_simple_pull_u8(buf);
	motor->seqNumber = net_buf_simple_pull_u8(buf);
	// Invoke status handler if present
	if (motor->handlers->forwardToUart) {
		motor->handlers->forwardToUart(false, ctx->addr, MOTOR_TYPE, STATUS_ID, data,
					       sizeof(data));
	}
	return 0;
}

const struct bt_mesh_model_op btMeshMotorOp[] = {
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_CODE,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_CODE), handleStatusCode },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ID, BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ID),
	  handleStatusId },
	{ BT_MESH_MODEL_MOTOR_OP_STATUS_ALL,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_ALL), handleStatusAll },
	BT_MESH_MODEL_OP_END,
};
//todo
static int btMeshMotorUpdateHandler(struct bt_mesh_model *model)
{
	LOG_DBG("MotorUpdateHandler");
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

const struct btMeshMotorHandlers motorHandlers = {
	.forwardToUart = forwardToUart,
};
