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

LOG_MODULE_REGISTER(vnd_activation, LOG_LEVEL_INF);
#define ACTIVATION_TIMER 10

static int encodeStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    struct btMeshActivation *activation = model->user_data;
    BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_ACTIVATION_OP_STATUS,
                             BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS);

    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_ACTIVATION_OP_STATUS);
    net_buf_simple_add_u8(&msg, activation->timerIsActive);
    net_buf_simple_add_le32(&msg, activation->timeRemaining);

    LOG_INF("Send [activation][STATUS] op[0x%06x] from addr:0x%04x. to :0x%04x. tid:-- ", BT_MESH_MODEL_ACTIVATION_OP_STATUS, bt_mesh_model_elem(activation->model)->addr, ctx->addr);
    return bt_mesh_model_send(activation->model, ctx, &msg, NULL, NULL);
}

int sendActivationGetStatus(struct btMeshActivation *activation,
                            uint16_t addr)
{
    struct bt_mesh_msg_ctx ctx = {
        .addr = addr,
        .app_idx = activation->model->keys[0],
        .send_ttl = BT_MESH_TTL_DEFAULT,
        .send_rel = true,
    };

    BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_ACTIVATION_OP_STATUS_GET,
                             BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS_GET);
    bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_ACTIVATION_OP_STATUS_GET);

    LOG_INF("Send [activation][GET] op[0x%06x] from addr:0x%04x. to :0x%04x. tid:-- ", BT_MESH_MODEL_ACTIVATION_OP_STATUS_GET, bt_mesh_model_elem(activation->model)->addr, ctx.addr);
    return bt_mesh_model_send(activation->model, &ctx, &buf, NULL, NULL);
}

int sendActivationSetPwd(struct btMeshActivation *activation, uint16_t addr, uint16_t pwd, bool timerIsActive)
{
    struct bt_mesh_msg_ctx ctx = {
        .addr = addr,
        .app_idx = activation->model->keys[0],
        .send_ttl = BT_MESH_TTL_DEFAULT,
        .send_rel = true,
    };

    BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_ACTIVATION_OP_PWD_SET,
                             BT_MESH_MODEL_ACTIVATION_OP_LEN_PWD_SET);
    bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_ACTIVATION_OP_PWD_SET);

    net_buf_simple_add_le16(&buf, pwd);
    net_buf_simple_add_u8(&buf, timerIsActive);
    LOG_INF("Send [activation][SETPWD] op[0x%06x] from addr:0x%04x. to :0x%04x. tid:-- ", BT_MESH_MODEL_ACTIVATION_OP_PWD_SET, bt_mesh_model_elem(activation->model)->addr, ctx.addr);

    return bt_mesh_model_send(activation->model, &ctx, &buf, NULL, NULL);
}

static int handleSetPwd(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    LOG_INF("Received [activation][SETPWD] op[0x%06x] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ", model->op->opcode, ctx->addr, ctx->recv_dst, ctx->recv_rssi);
    uint16_t pwd = net_buf_simple_pull_le16(buf);
    bool timerIsActive = net_buf_simple_pull_u8(buf);

    struct btMeshActivation *activation = model->user_data;

    if (activation->handlers->setPwd)
    {
        activation->handlers->setPwd(activation, timerIsActive, pwd);
    }

    return encodeStatus(model, ctx);
}

static int handleStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    LOG_INF("Received [activation][STATUS] op[0x%06x] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ", model->op->opcode, ctx->addr, ctx->recv_dst, ctx->recv_rssi);

    // Extract data from buffer
    uint8_t timerIsActive = net_buf_simple_pull_u8(buf);
    uint32_t timeRemaining = net_buf_simple_pull_le32(buf);

    // Log result to console
    LOG_DBG("Result timerIsActive: [%s] Time remaining: [%d]", timerIsActive ? "true" : "false", timeRemaining);

    // Get user data from model
    struct btMeshActivation *activation = model->user_data;

    // Invoke status handler if present
    if (activation->handlers->status)
    {
        activation->handlers->status(model, ctx, buf);
    }
    return 0;
}

static int handleStatusGet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
                           struct net_buf_simple *buf)
{

    LOG_INF("Received [activation][GET] op[0x%06x] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:-- ", model->op->opcode, ctx->addr, ctx->recv_dst, ctx->recv_rssi);

    return encodeStatus(model, ctx);
}

const struct bt_mesh_model_op btMeshActivationOp[] = {
    {BT_MESH_MODEL_ACTIVATION_OP_PWD_SET,
     BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_PWD_SET),
     handleSetPwd},
    {BT_MESH_MODEL_ACTIVATION_OP_STATUS_GET,
     BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS_GET),
     handleStatusGet},
    {BT_MESH_MODEL_ACTIVATION_OP_STATUS,
     BT_MESH_LEN_EXACT(BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS),
     handleStatus},
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

    return 0;
}

static int btMeshActivationStart(struct bt_mesh_model *model)
{
    LOG_DBG("Activation start");

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

static int activationSetPwd(struct btMeshActivation *activation, bool timerIsActive,
                            uint8_t pwd)
{

    // transfer pwd vi auart
    return 0;
}

static int activationStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    return 0;
}

const struct btMeshActivationHandlers activationHandlers = {
    .setPwd = activationSetPwd,
    .status = activationStatus,
};