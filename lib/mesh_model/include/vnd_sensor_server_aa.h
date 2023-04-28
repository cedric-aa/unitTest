
#ifndef VND_SENSOR_SERVER_AA_H__
#define VND_SENSOR_SERVER_AA_H__

#include "vnd_sensor_aa.h"

/** Bluetooth Mesh sensor model handlers. */
struct btMeshSensorHandlers {
	int (*const updateTemp)(struct btMeshSensor *sensor);
};

int updateTemp(struct btMeshSensor *sensor);


#endif /* VND_SENSOR_AA_H__ */