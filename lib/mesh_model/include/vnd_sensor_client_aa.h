
#ifndef VND_SENSOR_CLEINT_AA_H__
#define VND_SENSOR_CLIENT_AA_H__

#include "vnd_sensor_aa.h"

/** Bluetooth Mesh sensor model handlers. */
struct btMeshSensorHandlers {
	int (*const forwardToUart)(uint8_t uartAck, uint16_t addr, uint8_t messageType,
				   uint8_t messageId, uint8_t *buf, uint8_t len);
};

int sendSensorGetStatus(struct btMeshSensor *sensor, uint16_t addr, uint8_t seqNum);



#endif /* VND_SENSOR_AA_H__ */