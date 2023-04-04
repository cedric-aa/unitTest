#include <zephyr/bluetooth/mesh.h>
#include "vnd_sensor_client_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include <bluetooth/mesh/sensor_types.h>
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(vnd_sensor, LOG_LEVEL_INF);


int sendSensorGet(struct btMeshSensor *sensor, uint16_t addr, uint8_t seqNumber)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = addr,
		.app_idx = sensor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, BT_MESH_MODEL_SENSOR_OP_GET, BT_MESH_MODEL_SENSOR_OP_LEN_GET);
	bt_mesh_model_msg_init(&buf, BT_MESH_MODEL_SENSOR_OP_GET);
	net_buf_simple_add_u8(&buf, seqNumber);

	LOG_INF("Send [SENSOR_TYPE][GET] to 0x%04x. sequenceNumber:%d ", ctx.addr, seqNumber);
	return bt_mesh_model_send(sensor->model, &ctx, &buf, NULL, NULL);
}

static int handleStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	LOG_INF("Received [SENSOR_TYPE][STATUS] from addr:0x%04x rssi:%d SequenceNumber:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshSensor *sensor = model->user_data;
	
	if (sensor->handlers->forwardToUart) {
		sensor->handlers->forwardToUart(false, ctx->addr, SENSOR_TYPE, STATUS, buf->data,
						buf->len);
	}
	return 0;
}

const struct bt_mesh_model_op btMeshSensorOp[] = {
	{ BT_MESH_MODEL_SENSOR_OP_STATUS, BT_MESH_LEN_EXACT(BT_MESH_MODEL_SENSOR_OP_LEN_STATUS),
	  handleStatus },
	BT_MESH_MODEL_OP_END,
};

static int btMeshSensorUpdateHandler(struct bt_mesh_model *model)
{
	LOG_INF("UpdanteHandler");
	return 0;
}

static int btMeshSensorInit(struct bt_mesh_model *model)
{
	struct btMeshSensor *sensor = model->user_data;
	sensor->model = model;
	net_buf_simple_init_with_data(&sensor->pubMsg, sensor->buf, sizeof(sensor->buf));
	sensor->pub.msg = &sensor->pubMsg;
	sensor->pub.update = btMeshSensorUpdateHandler;

	return 0;
}

static int btMeshSensorStart(struct bt_mesh_model *model)
{
	LOG_DBG("sensor start");

	return 0;
}

static void btMeshSensorReset(struct bt_mesh_model *model)
{
	LOG_DBG("Reset sensor model");
	
}

const struct bt_mesh_model_cb btMeshSensorCb = {
	.init = btMeshSensorInit,
	.start = btMeshSensorStart,
	.reset = btMeshSensorReset,
};

const struct btMeshSensorHandlers sensorHandlers = {
	.forwardToUart = forwardToUart,
};