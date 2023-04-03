#include <zephyr/bluetooth/mesh.h>
#include "vnd_sensor_server_aa.h"
#include "mesh/net.h"
#include <string.h>
#include <zephyr/logging/log.h>
#include "storage_aa.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <bluetooth/mesh/sensor_types.h>
#include <bluetooth/mesh/models.h>

LOG_MODULE_REGISTER(vnd_sensor, LOG_LEVEL_INF);

#define SENSOR_NODE DT_NODELABEL(temp)
#define SENSOR_DATA_TYPE SENSOR_CHAN_DIE_TEMP

static const struct device *dev = DEVICE_DT_GET(SENSOR_NODE);

int updateTemp(struct btMeshSensor *sensor)
{
	struct bt_mesh_msg_ctx ctx = {
		.addr = 0xc000,
		.app_idx = sensor->model->keys[0],
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.send_rel = true,
	};

	struct sensor_value rsp;
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_DATA_TYPE, &rsp);

	// Make sure the integer part is within the range of [0, 255]
	sensor->val1 = (uint8_t)((rsp.val1 < 0) ? 0 : (rsp.val1 > 255) ? 255 : rsp.val1);
	sensor->val2 = rsp.val2 / 10000;
	LOG_INF("sensor1 value %d,%d", sensor->val1, sensor->val2);
	LOG_INF("sensor2 value %d,%d", rsp.val1, rsp.val2);

	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_SENSOR_OP_STATUS,
				 BT_MESH_MODEL_SENSOR_OP_LEN_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_SENSOR_OP_STATUS);
	net_buf_simple_add_u8(&msg, sensor->val1);
	net_buf_simple_add_u8(&msg, sensor->val2);
	net_buf_simple_add_u8(&msg, sensor->sequenceNumber++);

	if (!bt_mesh_model_send(sensor->model, &ctx, &msg, NULL, NULL)) {
		LOG_INF("Send [SENSOR_TYPE][STATUS] to 0x%04x. sequenceNumber:%d ", ctx.addr,
			sensor->sequenceNumber);
		return 0;
	} else {
		LOG_ERR("ERROR Send [SENSOR_TYPE][STATUS] to 0x%04x. sequenceNumber:%d ", ctx.addr,
			sensor->sequenceNumber);
		return -1;
	}

	return 0;
}
/*
static int encodeStatus(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, uint8_t seqNumber)
{
	struct btMeshSensor *sensor = model->user_data;
	BT_MESH_MODEL_BUF_DEFINE(msg, BT_MESH_MODEL_SENSOR_OP_STATUS,
				 BT_MESH_MODEL_SENSOR_OP_LEN_STATUS);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_SENSOR_OP_STATUS);
	net_buf_simple_add_u8(&msg, sensor->val1);
	net_buf_simple_add_u8(&msg, sensor->val2);
	net_buf_simple_add_u8(&msg, sensor->sequenceNumber++);

	if (!bt_mesh_model_send(sensor->model, ctx, &msg, NULL, NULL)) {
		LOG_INF("Send [SENSOR_TYPE][STATUS] to 0x%04x. sequenceNumber:%d ", ctx->addr,
			seqNumber);
		return 0;
	} else {
		LOG_ERR("ERROR Send [SENSOR_TYPE][STATUS] to 0x%04x. sequenceNumber:%d ", ctx->addr,
			seqNumber);
		return -1;
	}

	return 0;
}
*/
static int handleStatusGet(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			   struct net_buf_simple *buf)
{
	LOG_INF("Received [SENSOR_TYPE][GET] from addr:0x%04x. rssi:%d tid:%d ", ctx->addr,
		ctx->recv_rssi, buf->data[buf->len - 1]);

	struct btMeshSensor *sensor = model->user_data;
	updateTemp(sensor);
	//encodeStatus(model, ctx, buf->data[buf->len - 1]);

	return 0;
}

const struct bt_mesh_model_op btMeshSensorOp[] = {
	{ BT_MESH_MODEL_SENSOR_OP_GET, BT_MESH_LEN_EXACT(BT_MESH_MODEL_SENSOR_OP_LEN_GET),
	  handleStatusGet },
	BT_MESH_MODEL_OP_END,
};

static int btMeshSensorUpdateHandler(struct bt_mesh_model *model)
{
	return 0;
}

static int btMeshSensorInit(struct bt_mesh_model *model)
{
	struct btMeshSensor *sensor = model->user_data;
	sensor->model = model;
	net_buf_simple_init_with_data(&sensor->pubMsg, sensor->buf, sizeof(sensor->buf));
	sensor->pub.msg = &sensor->pubMsg;
	sensor->pub.update = btMeshSensorUpdateHandler;
	sensor->sequenceNumber = 0;

	if (!device_is_ready(dev)) {
		printk("Temperature sensor not ready\n");
	} else {
		printk("Temperature sensor (%s) initiated\n", dev->name);
	}

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
	//   struct btMeshSensor *sensor = model->user_data;
}

const struct bt_mesh_model_cb btMeshSensorCb = {
	.init = btMeshSensorInit,
	.start = btMeshSensorStart,
	.reset = btMeshSensorReset,
};

const struct btMeshSensorHandlers sensorHandlers = {
	.updateTemp = updateTemp,

};