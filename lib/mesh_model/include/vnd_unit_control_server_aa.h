/**
 * @file
 * @defgroup Mesh_CB
 * @{
 * @brief API for the Bluetooth Mesh custom/vendor model communicate with the unitControl.
 */
#ifndef VND_UNIT_CONTROL_SERVER_AA_H__
#define VND_UNIT_CONTROL_SERVER_AA_H__

#include "vnd_unit_control_aa.h"

struct btMeshUnitControlHandlers {
	void (*const fullCmdSet)(uint8_t *buff, uint8_t len);
};

void sendUnitControlFullCmdSetAck(struct btMeshUnitControl *unitControl, uint8_t result);
void unitControlUpdateStatus(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize);
void sendToCbUartStatus();

extern struct btMeshUnitControl unitControl;

extern struct k_timer setAckTimer;
extern struct k_timer updateTimer;

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshUnitControlOp[];
extern const struct bt_mesh_model_cb btMeshUnitControlCb;
extern const struct btMeshUnitControlHandlers unitControlHandlers;
/** @endcond */

#endif /* VND_UNIT_CONTROL_AA_H__ */