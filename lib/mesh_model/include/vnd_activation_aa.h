/**
 * @file
 * @defgroup Mesh_CB
 * @{
 * @brief API for the Bluetooth Mesh custom/vendor Activation.
 */

#ifndef VND_ACTIVATION_AA_H__
#define VND_ACTIVATION_AA_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

/** Company ID and Model ID */
#define BT_MESH_MODEL_ACTIVATION_COMPANY_ID 0x0059
#define BT_MESH_CB_MODEL_ACTIVATION_ID 0x000B

/*Opcode*/
#define BT_MESH_MODEL_ACTIVATION_OP_GET BT_MESH_MODEL_OP_3(0x0A, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_ACTIVATION_OP_SET BT_MESH_MODEL_OP_3(0x0B, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_ACTIVATION_OP_STATUS BT_MESH_MODEL_OP_3(0x0C, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_ACTIVATION_OP_STATUS_CODE                                                    \
	BT_MESH_MODEL_OP_3(0x0D, BT_MESH_CB_MODEL_ACTIVATION_ID)

/*OpCode Len*/
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_GET 1
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_SET 4
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS 6
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS_CODE 2

/* Forward declaration of the Bluetooth Mesh Activation model context. */
struct btMeshActivation;

/** @def btMeshActivation
 *
 * @brief Bluetooth Mesh Activation model composition data entry.
 *
 * @param[in] _activation Pointer to a @ref btMeshActivation instance.
 */
#define BT_MESH_MODEL_ACTIVATION(_activation)                                                      \
	BT_MESH_MODEL_VND_CB(BT_MESH_MODEL_ACTIVATION_COMPANY_ID, BT_MESH_CB_MODEL_ACTIVATION_ID,  \
			     btMeshActivationOp, &(_activation)->pub,                              \
			     BT_MESH_MODEL_USER_DATA(struct btMeshActivation, _activation),        \
			     &btMeshActivationCb)

/** Bluetooth Mesh Activation model handlers. */
struct btMeshActivationHandlers {
	int (*const forwardToUart)(uint8_t uartAck, uint16_t addr, uint8_t messageType,
				   uint8_t messageId, uint8_t *buf, uint8_t len);
};

/**
 * Bluetooth Mesh Activation model context.
 */
struct btMeshActivation {
	/** Access model pointer. */
	struct bt_mesh_model *model;
	struct bt_mesh_model_pub pub;
	struct net_buf_simple pubMsg;
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_MODEL_ACTIVATION_OP_STATUS,
					  BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS)];
	const struct btMeshActivationHandlers *handlers;

	// Added fields
	uint8_t timerState;
	uint16_t pwd;
	uint32_t lockOutDay;
	uint8_t seqNumber;
};

int sendActivationGetStatus(struct btMeshActivation *activation, uint16_t addr, uint8_t seqNum);
int sendActivationSetPwd(struct btMeshActivation *activation, uint16_t addr, uint8_t *buffer,
			 size_t len);
int sendActivationStatusCode(struct btMeshActivation *activation, uint16_t addr, uint8_t statusCode,
			     uint8_t seqNum);
void activationUpdateStatus(struct btMeshActivation *activation, uint8_t *buf, size_t bufSize);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshActivationOp[];
extern const struct bt_mesh_model_cb btMeshActivationCb;
extern const struct btMeshActivationHandlers activationHandlers;
/** @endcond */

#endif /* VND_ACTIVATION_AA_H__ */