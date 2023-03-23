/**
 * @file
 * @defgroup Mesh_CB
 * @{
 * @brief API for the Bluetooth Mesh custom/vendor model communicate with the unitControl.
 */

#ifndef VND_UNIT_CONTROL_CLIENT_AA_H__
#define VND_UNIT_CONTROL_CLIENT_AA_H__

#include "vnd_unit_control_aa.h"

/** Bluetooth Mesh unitControl model handlers. */
struct btMeshUnitControlHandlers {
	void (*const fullCmd)(struct bt_mesh_msg_ctx *ctx, uint8_t *buff, uint8_t len);
	void (*const statusCode)(struct btMeshUnitControl *unitControl, struct bt_mesh_msg_ctx *ctx,
				 uint8_t statusCode);
};

int sendUnitControlFullCmdGet(struct btMeshUnitControl *unitControl, uint16_t addr);
int sendUnitControlFullCmdSet(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize,
			      uint16_t address);

#endif /* VND_UNIT_CONTROL_AA_H__ */