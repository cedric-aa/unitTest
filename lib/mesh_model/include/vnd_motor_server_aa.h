#ifndef VND_MOTOR_SERVER_AA_H__
#define VND_MOTOR_SERVER_AA_H__

#include "vnd_motor_aa.h"

void motorUpdateStatus(struct btMeshMotor *motor, uint8_t *buf, size_t bufSize);
void motorUpdateStatusId(struct btMeshMotor *motor, uint8_t id, uint8_t level);
int sendMotorStatusCode(struct btMeshMotor *motor, uint16_t addr, uint8_t statusCode,
			uint8_t seqNum);

extern struct btMeshMotor motor;

extern struct k_timer motorSetAckTimer;
extern struct k_timer motorUpdateTimer;


#endif /* VND_ACTIVATION_AA_H__ */