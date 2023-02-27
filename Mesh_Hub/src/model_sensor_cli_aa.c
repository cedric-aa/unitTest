#include "model_sensor_cli_aa.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sensor_cli_AA, LOG_LEVEL_INF);

static void sensorCliDataCb(struct bt_mesh_sensor_cli *cli,
                            struct bt_mesh_msg_ctx *ctx,
                            const struct bt_mesh_sensor_type *sensor,
                            const struct sensor_value *value)
{
    if (sensor->id == bt_mesh_sensor_present_dev_op_temp.id)
    {
        LOG_INF("Received [Sensor][Temp] op[0x%06x] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:", cli->model->op->opcode, ctx->addr, ctx->recv_dst, ctx->recv_rssi);
    }
}

const struct bt_mesh_sensor_cli_handlers sensorCliHandlers = {
    .data = sensorCliDataCb,
};
