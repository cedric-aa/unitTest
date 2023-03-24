/**
 * @file
 * @defgroup Mesh_CB
 * @{
 * @brief API for the Bluetooth Mesh custom/vendor MOTOR.
 */

#ifndef VND_MOTOR_AA_H__
#define VND_MOTOR_AA_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

/** Company ID and Model ID */
#define BT_MESH_MODEL_MOTOR_COMPANY_ID 0x0059
#define BT_MESH_CB_MODEL_MOTOR_ID 0x000C

/*Opcode*/
#define BT_MESH_MODEL_MOTOR_OP_GET BT_MESH_MODEL_OP_3(0x0A, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_MOTOR_OP_SET BT_MESH_MODEL_OP_3(0x0B, BT_MESH_CB_MODEL_ACTIVATION_ID)
#define BT_MESH_MODEL_MOTOR_OP_STATUS BT_MESH_MODEL_OP_3(0x0C, BT_MESH_CB_MODEL_ACTIVATION_ID)

/*OpCode Len*/
#define BT_MESH_MODEL_MOTOR_OP_LEN_STATUS_GET 0
#define BT_MESH_MODEL_MOTOR_OP_LEN_SET 3
#define BT_MESH_MODEL_MOTOR_OP_LEN_STATUS 5

/* Forward declaration of the Bluetooth Mesh Motor model context. */
struct btMeshMotor;

/** @def btMeshMotor
 *
 * @brief Bluetooth Mesh Motor model composition data entry.
 *
 * @param[in] _motor Pointer to a @ref btMeshMotor instance.
 */
#define BT_MESH_MODEL_MOTOR(_motor)                                                                \
	BT_MESH_MODEL_VND_CB(BT_MESH_MODEL_MOTOR_COMPANY_ID, BT_MESH_CB_MODEL_MOTOR_ID,            \
			     btMeshMotorOp, &(_motor)->pub,                                        \
			     BT_MESH_MODEL_USER_DATA(struct btMeshMotor, _motor), &btMeshMotorCb)

/** Bluetooth Mesh  model handlers. */
struct btMeshMotorHandlers {
	/** @brief Handler for a mode message.
     *
     * @param[in] motor instance 
     * @param[in] ctx Context of the incoming message.
     */
	int (*const set)(struct btMeshMotor *motor, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a Get Status message.
     *
     * @param[in]  Activation instance
     * @param[in] ctx Context of the incoming message.
     */
	int (*const status)(struct btMeshActivation *activation, struct bt_mesh_msg_ctx *ctx);
};

/**
 * Bluetooth Mesh Motor model context.
 */
struct btMeshMotor {
	/** Access model pointer. */
	struct bt_mesh_model *model;
	struct bt_mesh_model_pub pub;
	struct net_buf_simple pubMsg;
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_MODEL_MOTOR_OP_STATUS,
					  BT_MESH_MODEL_MOTOR_OP_LEN_STATUS)];
	const struct btMeshMotorHandlers *handlers;

	// Added fields
	uint8_t motorLevel[10];
	uint8_t seqNumber;
};

int sendSetIdMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t lvl, uint8_t id,
			uint8_t seqNum);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshMotorOp[];
extern const struct bt_mesh_model_cb btMeshMotorCb;
extern const struct btMeshMotorHandlers motorHandlers;
/** @endcond */

#endif /* VND_ACTIVATION_AA_H__ */