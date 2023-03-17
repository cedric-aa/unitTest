#ifndef MODEL_LEVEL_CLI_AA_H__
#define MODEL_LEVEL_CLI_AA_H__

#include <bluetooth/mesh/gen_lvl_cli.h>

int sendGetLvl(struct bt_mesh_lvl_cli level_cli);
int sendSetLvl(struct bt_mesh_lvl_cli level_cli);
extern void levelMotorStatusHandler(struct bt_mesh_lvl_cli *cli, struct bt_mesh_msg_ctx *ctx,
				    const struct bt_mesh_lvl_status *status);

#endif