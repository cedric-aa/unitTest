
#ifndef VND_SENSOR_SERVER_AA_H__
#define VND_SENSOR_SERVER_AA_H__

#include "vnd_sensor_aa.h"

/** Bluetooth Mesh sensor model handlers. */
struct btMeshSensorHandlers {
	int (*const updateTemp)(struct btMeshSensor *sensor);
};

int updateTemp(struct btMeshSensor *sensor);

/** @cond INTERNAL_HIDDEN */
extern const struct bt_mesh_model_op btMeshSensorOp[];
extern const struct bt_mesh_model_cb btMeshSensorCb;
extern const struct btMeshSensorHandlers sensorHandlers;
/** @endcond */

#endif /* VND_SENSOR_AA_H__ */