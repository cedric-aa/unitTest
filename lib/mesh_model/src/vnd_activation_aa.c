/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/bluetooth/mesh.h>
#include "vnd_activation_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_activation, LOG_LEVEL_INF);

static bool isUpdated;

static void sendToCbUartActivationStatus()
{
	static uint8_t seqNumber;
	LOG_INF("send [UART][MOTOR_TYPE][GET] seqNumber %d", seqNumber);
	dataQueueItemType uartTxQueueItem = headerFormatUartTx(0x01ad, ACTIVATION_TYPE, GET, true);
	uartTxQueueItem.bufferItem[uartTxQueueItem.length++] = seqNumber++;
	uartTxQueueItem.bufferItem[0] = uartTxQueueItem.length - 1; // update lenghtpayload

	k_msgq_put(&uartTxQueue, &uartTxQueueItem, K_NO_WAIT);
}

void activationUpdateStatus(struct btMeshActivation *activation, uint8_t *buf, size_t bufSize)
{
	isUpdated = true;
	activation->timerState = buf[0];
	memcpy(&activation->dayRemaining, &buf[1], sizeof(uint16_t));
}

int sendActivationStatusCode(struct btMeshActivation *activation, uint16_t addr, uint8_t statusCode,
			     uint8_t seqNum)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = activation->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_ACTIVATION_OP_STATUS_CODE,
				 BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS_CODE);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_ACTIVATION_OP_STATUS_CODE);
	net_buf_simple_add_u8(&msg, statusCode);
	net_buf_simple_add_u8(&msg, seqNum);

	if (!bt_mesh_model_send(activation->model, &ctx, &msg, NULL, NULL)) {
		LOG_INF("Send [ACTIVATION_TYPE][STATUS_CODE] to 0x%04x seqNum:%d ", ctx.addr,
			seqNum);
		return 0;

	} else {
		LOG_ERR("ERROR Send [ACTIVATION_TYPE][STATUS_CODE] to 0x%04x seqNum:%d", ctx.addr,
			seqNum);

		return -1;
	}
	return 0;
}

static int encodeStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t seqNumber)
{
	struct btMeshActivation *activation = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_ACTIVATION_OP_STATUS,
				 BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_ACTIVATION_OP_STATUS);
	net_buf_simple_add_u8(&msg, activation->timerState);
	net_buf_simple_add_u8(&msg, activation->dayRemaining);

	if (!bt_mesh_model_send(activation->model, ctx, &msg, NULL, NULL)) {
		LOG_INF("Send [ACTIVATION_TYPE][STATUS] to 0x%04x. sequenceNumber:%d ", ctx->addr,
			seqNumber);
		return 0;
	} else {
		LOG_ERR("ERROR Send [ACTIVATION_TYPE][STATUS] to 0x%04x. sequenceNumber:%d ",
			ctx->addr, seqNumber);
		return -1;
	}

	return 0;
}

//client side
int sendActivationGetStatus(struct btMeshActivation *activation, uint16_t addr, uint8_t seqNumber)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = activation->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_ACTIVATION_OP_GET,
				 BT_MESH_MODEL_ACTIVATION_OP_LEN_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_ACTIVATION_OP_GET);
	net_buf_simple_add_u8(&buf, seqNumber);

	LOG_INF("Send [ACTIVATION_TYPE][GET] to 0x%04x. sequenceNumber:%d ", ctx.addr, seqNumber);
	return bt_mesh_model_send(activation->model, &ctx, &buf, NULL, NULL);
}

//client side
int sendActivationSetPwd(struct btMeshActivation *activation, uint16_t address, uint8_t *buffer,
			 size_t len)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = address,
		.app_idx = activation->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_ACTIVATION_OP_SET,
				 BT_MESH_MODEL_ACTIVATION_OP_LEN_SET);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_ACTIVATION_OP_SET);
	net_buf_simple_add_u8(&buf, buffer[0]); // timerState
	net_buf_simple_add_le16(&buf, *(uint16_t *)&buffer[1]); // password
	net_buf_simple_add_u8(&buf, buffer[3]); // lockOutDay
	net_buf_simple_add_u8(&buf, buffer[4]); // sequenceNumber
	LOG_INF("Send [ACTIVATION_TYPE][SET] to 0x%04x. seqNumber:%d ", ctx.addr, buffer[4]);

	return bt_mesh_model_send(activation->model, &ctx, &buf, NULL, NULL);
}

//server Side
static int handleSetPwd(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	LOG_INF("Received [ACTIVATION_TYPE][SET] from addr:0x%04x  rssi:%d sequenceNumber:%d",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshActivation *activation = model->user_data;

	//testing removed it after
	sendActivationStatusCode(activation, ctx->addr, WAITING_FOR_RESPONSE,
				 buf->data[buf->len - 1]);

	if (activation->handlers->forwardToUart) {
		activation->handlers->forwardToUart(false, ctx->addr, ACTIVATION_TYPE, SET,
						    buf->data, buf->len);
	}

	return 0;
}

//client Side
static int handleStatusCode(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [ACTIVATION_TYPE][STATUS_CODE] from addr:0x%04x rssi:%d sequenceNumber:%d ",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshActivation *activation = model->user_data;

	if (activation->handlers->forwardToUart) {
		activation->handlers->forwardToUart(false, ctx->addr, ACTIVATION_TYPE, STATUS_CODE,
						    buf->data, buf->len);
	}

	return 0;
}

static int handleStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	LOG_INF("Received [ACTIVATION_TYPE][STATUS] from addr:0x%04x rssi:%d sequenceNumber:%d ",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);

	// Get user data from model
	struct btMeshActivation *activation = model->user_data;

	if (activation->handlers->forwardToUart) {
		activation->handlers->forwardToUart(false, ctx->addr, ACTIVATION_TYPE, STATUS,
						    buf->data, buf->len);
	}
	return 0;
}

//server side
static int handleStatusGet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	LOG_INF("Received [ACTIVATION_TYPE][GET] from addr:0x%04x. rssi:%d sequenceNumber:%d ",
		ctx->addr, ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshActivation *activation = model->user_data;

	if (isUpdated) {
		encodeStatus(model, ctx, buf->data[buf->len - 1]);
	} else {
		sendActivationStatusCode(activation, ctx->addr, WAITING_FOR_RESPONSE, buf->data[buf->len - 1]);
		sendToCbUartActivationStatus();
	}

	return 0;
}

const struct bt_mesh_model_op btMeshActivationOp[] = {
	{ BT_MESH_MODEL_ACTIVATION_OP_SET, BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_SET),
	  handleSetPwd },
	{ BT_MESH_MODEL_ACTIVATION_OP_STATUS_CODE,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS_CODE), handleStatusCode },
	{ BT_MESH_MODEL_ACTIVATION_OP_GET, BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_GET),
	  handleStatusGet },
	{ BT_MESH_MODEL_ACTIVATION_OP_STATUS,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS), handleStatus },
	BT_MESH_MODEL_OP_END,
};

static int btMeshActivationUpdateHandler(struct bt_mesh_model *model)
{
	return 0;
}

static int btMeshActivationInit(struct bt_mesh_model *model)
{
	struct btMeshActivation *activation = model->user_data;
	activation->model = model;
	net_buf_simple_init_with_data(&activation->pubMsg, activation->buf,
				      sizeof(activation->buf));
	activation->pub.msg = &activation->pubMsg;
	activation->pub.update = btMeshActivationUpdateHandler;
	isUpdated = false;

	return 0;
}

static int btMeshActivationStart(struct bt_mesh_model *model)
{
	LOG_INF("Activation start");
	sendToCbUartActivationStatus();

	return 0;
}

static void btMeshActivationReset(struct bt_mesh_model *model)
{
	LOG_DBG("Reset activation model");
	//   struct btMeshActivation *activation = model->user_data;
}

const struct bt_mesh_model_cb btMeshActivationCb = {
	.init = btMeshActivationInit,
	.start = btMeshActivationStart,
	.reset = btMeshActivationReset,
};

const struct btMeshActivationHandlers activationHandlers = {
	.forwardToUart = forwardToUart,

};