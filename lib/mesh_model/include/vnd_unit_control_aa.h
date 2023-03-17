/**
 * @file
 * @defgroup Mesh_CB
 * @{
 * @brief API for the Bluetooth Mesh custom/vendor model communicate with the unitControl.
 */

#ifndef VND_UNIT_CONTROL_AA_H__
#define VND_UNIT_CONTROL_AA_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

/** Company ID and Model ID*/
#define BT_MESH_MODEL_UNIT_CONTROL_COMPANY_ID 0x0059
#define BT_MESH_CB_MODEL_UNIT_CONTROL_ID 0x000A

/** full Cmd to Unit Control message opcode.  */
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE                                             \
	BT_MESH_MODEL_OP_3(0x0F, BT_MESH_CB_MODEL_UNIT_CONTROL_ID)
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_GET                                         \
	BT_MESH_MODEL_OP_3(0x10, BT_MESH_CB_MODEL_UNIT_CONTROL_ID)
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET                                         \
	BT_MESH_MODEL_OP_3(0x0E, BT_MESH_CB_MODEL_UNIT_CONTROL_ID)
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE_SET_ACK                                     \
	BT_MESH_MODEL_OP_3(0x0D, BT_MESH_CB_MODEL_UNIT_CONTROL_ID)

/**length message**/
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE 8
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET 8
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_SET_ACK 1
#define BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE_GET 0

/** Bluetooth Mesh unitControl mode values. */
enum unitControlMode {
	UNIT_CONTROL_MODE_COOL = 1,
	UNIT_CONTROL_MODE_HEAT = 2,
	UNIT_CONTROL_MODE_FAN = 3,
	UNIT_CONTROL_MODE_DRY = 4,
	UNIT_CONTROL_MODE_AUTO = 5
};

enum unitControlCodeStatus {
	UNIT_CONTROL_CODE_STATUS_SUCCESS,
	UNIT_CONTROL_CODE_STATUS_FAIL,
	UNIT_CONTROL_CODE_STATUS_LOCK,
	UNIT_CONTROL_CODE_STATUS_TIMEOUT
};

/** Bluetooth Mesh unitControl OnOff values. */
enum unitControlFanSpeed {
	UNIT_CONTROL_FAN_SPEEP_1 = 1,
	UNIT_CONTROL_FAN_SPEEP_2 = 2,
	UNIT_CONTROL_FAN_SPEEP_3 = 3,
	UNIT_CONTROL_FAN_SPEEP_4 = 4,
	UNIT_CONTROL_FAN_SPEEP_5 = 5
};

/** Bluetooth Mesh unitControl temperature */
struct temperatureValue {
	/** Integer part of the value. */
	int8_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int8_t val2;
};

struct temperatureValues {
	struct temperatureValue targetTemp;
	struct temperatureValue currentTemp;
};

/* Forward declaration of the Bluetooth Mesh unitControl model context. */
struct btMeshUnitControl;

/** @def btMeshunitControl
 *
 * @brief Bluetooth Mesh unitControl model composition data entry.
 *
 * @param[in] _unitControl Pointer to a @ref btMeshUnitControl instance.
 */
#define BT_MESH_MODEL_UNIT_CONTROL(_unitControl)                                                   \
	BT_MESH_MODEL_VND_CB(BT_MESH_MODEL_UNIT_CONTROL_COMPANY_ID,                                \
			     BT_MESH_CB_MODEL_UNIT_CONTROL_ID, btMeshUnitControlOp,                \
			     &(_unitControl)->pub,                                                 \
			     BT_MESH_MODEL_USER_DATA(struct btMeshUnitControl, _unitControl),      \
			     &btMeshUnitControlCb)

/** Bluetooth Mesh unitControl model handlers. */
struct btMeshUnitControlHandlers {
	/** @brief Called after the node has been provisioned, or after all
	 * mesh data has been loaded from persistent storage.
	 *
	 * @param[in] cli unitControl instance that has been started.
	 */
	void (*const start)(struct btMeshUnitControl *unitControl);

	void (*const fullCmd)(struct btMeshUnitControl *unitControl, struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a fullCmdSet message.
	 *
	 * @param[in] cli unitControl instance that received the text message.
	 * @param[in] ctx Context of the incoming message.
	 * @param[in] onOff of a unitControl client published
	 * @param[in] fanSpeed of a unitControl client published
	 * @param[in] tempValues of a unitControl client published
	 * @param[in] unitControl of a unitControl client published
	 * the message.
	 */
	int8_t (*const fullCmdSet)(struct btMeshUnitControl *unitControl,
				   struct bt_mesh_msg_ctx *ctx);

	/** @brief Handler for a fullCmdSetAck message.
 *
 * @param[in] cli unitControl instance that received the text message.
 * @param[in] ctx Context of the incoming message.
 * @param[in] statusCode of a unitControl client published

 * the message.
 */
	void (*const fullCmdSetAck)(struct btMeshUnitControl *unitControl,
				    struct bt_mesh_msg_ctx *ctx, uint8_t status);
};

/**
 * Bluetooth Mesh unitControl model context.
 */
struct btMeshUnitControl {
	/** Access model pointer. */
	struct bt_mesh_model *model;
	/** Publish parameters. */
	struct bt_mesh_model_pub pub;
	/** Publication message. */
	struct net_buf_simple pubMsg;
	/** Publication message buffer. */
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_MESSAGE,
					  BT_MESH_MODEL_UNIT_CONTROL_FULL_CMD_OP_LEN_MESSAGE)];
	/** Handler function structure. */
	const struct btMeshUnitControlHandlers *handlers;
	/** Current Mode value. */
	enum unitControlMode mode;
	/**Current On/off value. */
	bool onOff;
	/**Current fan speed value. */
	enum unitControlFanSpeed fanSpeed;
	/**temperatures value. */
	struct temperatureValues tempValues;
	/**Unit control Type*/
	uint8_t unitControlType;
};

/** @brief Get current fullcmd value of a unitControl.
 *
 * @param[in] unitControl unitControl model instance to send the message.
 * @param[in] addr Address of the unitControl to get mode value of.
 *
 * @retval 0 Successfully sent the message.
 * @retval -EINVAL The model is not bound to an application key.
 * @retval -EAGAIN The device has not been provisioned.
 */
int sendUnitControlFullCmdGet(struct btMeshUnitControl *unitControl, uint16_t addr);

/** @param[in] unitControl  model instance to set the fullCmd.
 * @param unitControl
 * @param buf
 * @param bufSize
 * @retval 0 Successfully set the preceive and sent the message.
 * @retval -EADDRNOTAVAIL Publishing is not configured.
 * @retval -EAGAIN The device has not been provisioned.
 */
int sendUnitControlFullCmdSet(struct btMeshUnitControl *unitControl, uint8_t *buf, size_t bufSize);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshUnitControlOp[];
extern const struct bt_mesh_model_cb btMeshUnitControlCb;
extern const struct btMeshUnitControlHandlers unitControlHandlers;
/** @endcond */

#endif /* VND_UNIT_CONTROL_AA_H__ */