
#include "model_level_cli_aa.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(level_cli_AA, LOG_LEVEL_INF);

int sendGetLvl(struct bt_mesh_lvl_cli level_cli)
{
	LOG_INF("Send [level][GET] op[0x%06x] from addr:0x%04x.  tid:-- ", BT_MESH_LVL_OP_GET,
		bt_mesh_model_elem(level_cli.model)->addr);
	return bt_mesh_lvl_cli_get(&level_cli, NULL, NULL);
}

int sendSetLvl(struct bt_mesh_lvl_cli level_cli)
{
	// for testing
	struct bt_mesh_lvl_set set = {
		.lvl = 10, .transition = &(struct bt_mesh_model_transition){ .delay = 0, .time = 0 }
	};
	set.new_transaction = true;

	LOG_INF("Send [level][SET] op[0x%06x] from addr:0x%04x.  tid:-- ", BT_MESH_LVL_OP_SET,
		bt_mesh_model_elem(level_cli.model)->addr);
	return bt_mesh_lvl_cli_set(&level_cli, NULL, &set, NULL);
}

void levelMotorStatusHandler(struct bt_mesh_lvl_cli *cli, struct bt_mesh_msg_ctx *ctx,
			     const struct bt_mesh_lvl_status *status)
{
	LOG_INF("Received [level][Status] from addr:0x%04x. revc_addr:0x%04x. rssi:%d tid:%d",
		ctx->addr, ctx->recv_dst, ctx->recv_rssi, cli->tid);
	LOG_DBG("current:%d, target:%d", status->current, status->target);
}
