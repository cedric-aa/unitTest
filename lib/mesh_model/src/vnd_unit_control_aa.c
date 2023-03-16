/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/mesh.h>
#include "vnd_unit_control_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>

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

static void printClientStatus(struct btMeshUnitControl *unitControl)
{
	LOG_DBG("The mesh node is provisioned. The client address is 0x%04x.",
		bt_mesh_model_elem(unitControl->model)->addr);
	LOG_DBG("OnOff: [%s] ", unitControl->onOff ? "true" : "false");
	LOG_DBG("Mode: [%s]", modeToString[unitControl->mode]);
	LOG_DBG("Fan Speed: [%s] ", fanSpeedToString[unitControl->fanSpeed]);
	LOG_DBG("Current temperature: [%d,%d-C] ", unitControl->tempValues.currentTemp.val1,
		unitControl->tempValues.currentTemp.val2);
	LOG_DBG("Target temperature: [%d,%d-C] ", unitControl->tempValues.targetTemp.val1,
		unitControl->tempValues.targetTemp.val2);
	LOG_DBG("Unit Control Type: [%d] ", unitControl->unitControlType);
}

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

int sendUnitControlFullCmdGet(struct btMeshUnitControl *unitControl, uint16_t addr)
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

	LOG_INF("Send [unitControl][GET] op[0x%06x] from addr:0x%04x. to :0x%04x. tid:-- ",
		BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_GET,
		bt_mesh_model_elem(unitControl->model)->addr, ctx.addr);

	return bt_mesh_model_send(unitControl->model, &ctx, &buf, NULL, NULL);
}

int sendUnitControlFullCmdSet(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = (uint16_t)((buf[1] << 8) | buf[0]),
		.app_idx = unitControl->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET);
	for (int i = 2; i < bufSize; ++i) {
		net_buf_simple_add_u8(&msg, buf[i]);
	}

	LOG_INF("Send [unitControl][SET] op[0x%06x] from addr:0x%04x. to :0x%04x. tid:-- ",
		BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET,
		bt_mesh_model_elem(unitControl->model)->addr, ctx.addr);

	return bt_mesh_model_send(unitControl->model, &ctx, &msg, NULL, NULL);
}

static void sendUnitControlFullCmdSetAck(struct btMeshUnitControl *unitControl,
					 struct bt_mesh_msg_ctx *ctx, uint8_t result)
{
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET_ACK);
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK);

	net_buf_simple_add_u8(&msg, result);
	LOG_INF("Send [unitControl][SetAck] op[0x%06x] from addr:0x%04x. to :0x%04x. tid:-- ",
		BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK,
		bt_mesh_model_elem(unitControl->model)->addr, ctx->addr);

	(void)bt_mesh_model_send(unitControl->model, ctx, &msg, NULL, NULL);
}

static int handleFullCmd(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			 struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][STATUS] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi);

	struct btMeshUnitControl *unitControl = model->user_data;

	unitControl->mode = net_buf_simple_pull_u8(buf);
	unitControl->onOff = net_buf_simple_pull_u8(buf);
	unitControl->fanSpeed = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.currentTemp.val1 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.currentTemp.val2 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.targetTemp.val1 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.targetTemp.val2 = net_buf_simple_pull_u8(buf);
	unitControl->unitControlType = net_buf_simple_pull_u8(buf);
	printClientStatus(unitControl);

	if (unitControl->handlers->fullCmd) {
		unitControl->handlers->fullCmd(unitControl, ctx);
	}

	return 0;
}

static int handleFullCmdGet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][GET] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi);

	struct btMeshUnitControl *unitControl = model->user_data;

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE,
				 BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE);

	encodeFullCmd(&msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE, unitControl);

	LOG_INF("Send [unitControl][STATUS] op[0x%06x] from addr:0x%04x. revc_addr:0x%04x. tid:-- ",
		BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE, ctx->recv_dst, ctx->addr);

	(void)bt_mesh_model_send(unitControl->model, ctx, &msg, NULL, NULL);

	return 0;
}

static int handleFullCmdSetAck(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][SET-ACK] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi);
	struct btMeshUnitControl *unitControl = model->user_data;
	uint8_t status;

	status = net_buf_simple_pull_u8(buf);

	if (unitControl->handlers->fullCmdSetAck) {
		unitControl->handlers->fullCmdSetAck(unitControl, ctx, status);
	}

	return 0;
}

static int handleFullCmdSet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_INF("Received [unitControl][SET] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi);

	uint8_t result = 1;
	struct btMeshUnitControl *unitControl = model->user_data;

	unitControl->mode = net_buf_simple_pull_u8(buf);
	unitControl->onOff = net_buf_simple_pull_u8(buf);
	unitControl->fanSpeed = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.currentTemp.val1 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.currentTemp.val2 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.targetTemp.val1 = net_buf_simple_pull_u8(buf);
	unitControl->tempValues.targetTemp.val2 = net_buf_simple_pull_u8(buf);
	unitControl->unitControlType = net_buf_simple_pull_u8(buf);

	if (unitControl->handlers->fullCmdSet) {
		result = unitControl->handlers->fullCmdSet(unitControl, ctx);
	}

	sendUnitControlFullCmdSetAck(unitControl, ctx, result);

	return 0;
}

const struct bt_mesh_model_op btMeshUnitControlOp[] = {
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE), handleFullCmd },
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET),
	  handleFullCmdSet },
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET_ACK),
	  handleFullCmdSetAck },
	{ BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_GET,
	  BT_MESH_LEN_EXACT(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_GET),
	  handleFullCmdGet },
	BT_MESH_MODEL_OP_END,
};

static int btMeshUnitControlUpdateHandler(struct bt_mesh_model *model)
{
	LOG_DBG("UpdateHandler");
	struct btMeshUnitControl *unitControl = model->user_data;

	/* Continue publishing current state. */
	encodeFullCmd(model->pub->msg, BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE, unitControl);

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

	return 0;
}

static int btMeshUnitControlStart(struct bt_mesh_model *model)
{
	LOG_DBG("btMeshUnitControlStart");
	struct btMeshUnitControl *unitControl = model->user_data;

	if (unitControl->handlers->start) {
		unitControl->handlers->start(unitControl);
	}

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

static void unitControlFullCmd(struct btMeshUnitControl *unitControl, struct bt_mesh_msg_ctx *ctx)
{
	// Send to the HUB
}

static int8_t unitControlFullCmdSet(struct btMeshUnitControl *unitControl,
				    struct bt_mesh_msg_ctx *ctx)

{
	// TODO instead of passing in params those params use a buffer
	// TODO save into the flash the last command
	//  send via uart the fullCmd

	return 0;
}

static void unitControlHandleFullCmdSetAck(struct btMeshUnitControl *unitControl,
					   struct bt_mesh_msg_ctx *ctx, uint8_t ack)
{
	// TODO send via uart to the HUB
}

static void unitControlStart(struct btMeshUnitControl *unitControl)
{
	unitControl->mode = UNIT_CONTROL_MODE_COOL;
	unitControl->fanSpeed = UNIT_CONTROL_FAN_SPEEP_1;
}

const struct btMeshUnitControlHandlers unitControlHandlers = {
	.start = unitControlStart,
	.fullCmd = unitControlFullCmd,
	.fullCmdSet = unitControlFullCmdSet,
	.fullCmdSetAck = unitControlHandleFullCmdSetAck,
};
