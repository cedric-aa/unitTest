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
#define BT_MESH_MODEL_ACTIVATION_OP_STATUS_GET                                                     \
	BT_MESH_MODEL_OP_3(0x0A, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_ACTIVATION_OP_PWD_SET BT_MESH_MODEL_OP_3(0x0B, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_ACTIVATION_OP_STATUS BT_MESH_MODEL_OP_3(0x0C, BT_MESH_CB_MODEL_ACTIVATION_ID)

/*OpCode Len*/
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS_GET 0
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_PWD_SET 3
#define BT_MESH_MODEL_ACTIVATION_OP_LEN_STATUS 5

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
	/** @brief Handler for a mode message.
     *
     * @param[in] activation instance that received the text message.
     * @param[in] ctx Context of the incoming message.
     * @param[in] pwd of a Activation client published
     */
	int (*const setPwd)(struct btMeshActivation *activation, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a Get Status message.
     *
     * @param[in]  Activation instance that received the text message.
     * @param[in] ctx Context of the incoming message.
     * @param[in] buf net_buf_simple buffer including the status info
     */
	int (*const status)(struct btMeshActivation *activation, struct bt_mesh_msg_ctx *ctx);
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
	bool timerIsActive;
	uint16_t pwd;
	uint32_t timeRemaining;
	uint8_t seqNumber;
};

int sendActivationGetStatus(struct btMeshActivation *activation, uint16_t addr);
int sendActivationSetPwd(struct btMeshActivation *activation, uint16_t addr, uint8_t *buffer,
			 size_t len);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshActivationOp[];
extern const struct bt_mesh_model_cb btMeshActivationCb;
extern const struct btMeshActivationHandlers activationHandlers;
/** @endcond */

#endif /* VND_ACTIVATION_AA_H__ */