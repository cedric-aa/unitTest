#ifndef VND_SENSOR_AA_H__
#define VND_SENSOR_AA_H__

#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/model_types.h>

/** Company ID and Model ID */
#define BT_MESH_MODEL_SENSOR_COMPANY_ID 0x0059
#define BT_MESH_CB_MODEL_SENSOR_ID 0x000D

/*Opcode*/
#define BT_MESH_MODEL_SENSOR_OP_GET BT_MESH_MODEL_OP_3(0x0A, BT_MESH_CB_MODEL_SENSOR_ID)
#define BT_MESH_MODEL_SENSOR_OP_STATUS BT_MESH_MODEL_OP_3(0x0B, BT_MESH_CB_MODEL_SENSOR_ID)

/*OpCode Len*/
#define BT_MESH_MODEL_SENSOR_OP_LEN_GET 1
#define BT_MESH_MODEL_SENSOR_OP_LEN_STATUS 6

/* Forward declaration of the Bluetooth Mesh sensor model context. */
struct btMeshSensor;

/** @def btMeshSensor
 *
 * @brief Bluetooth Mesh sensor model composition data entry.
 *
 * @param[in] _sensor Pointer to a @ref btMeshSensor instance.
 */
#define BT_MESH_MODEL_SENSOR(_sensor)                                                              \
	BT_MESH_MODEL_VND_CB(BT_MESH_MODEL_SENSOR_COMPANY_ID, BT_MESH_CB_MODEL_SENSOR_ID,          \
			     btMeshSensorOp, &(_sensor)->pub,                                      \
			     BT_MESH_MODEL_USER_DATA(struct btMeshSensor, _sensor),                \
			     &btMeshSensorCb)

/** Bluetooth Mesh sensor model handlers. */
struct btMeshSensorHandlers {
	int (*const updateTemp)(struct bt_mesh_model *model);

	int (*const forwardToUart)(uint8_t uartAck, uint16_t addr, uint8_t messageType,
				   uint8_t messageId, uint8_t *buf, uint8_t len);
};

/**
 * Bluetooth Mesh sensor model context.
 */
struct btMeshSensor {
	/** Access model pointer. */
	struct bt_mesh_model *model;
	struct bt_mesh_model_pub pub;
	struct net_buf_simple pubMsg;
	uint8_t buf[BT_MESH_MODEL_BUF_LEN(BT_MESH_MODEL_SENSOR_OP_STATUS,
					  BT_MESH_MODEL_SENSOR_OP_LEN_STATUS)];
	const struct btMeshSensorHandlers *handlers;

	// Added fields
	uint8_t val1;
	uint8_t val2;
	//uint8_t location;
	uint8_t sequenceNumber;
};

int sendSensorGetStatus(struct btMeshSensor *sensor, uint16_t addr, uint8_t seqNum);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshSensorOp[];
extern const struct bt_mesh_model_cb btMeshSensorCb;
extern const struct btMeshSensorHandlers sensorHandlers;
/** @endcond */

#endif /* VND_SENSOR_AA_H__ */