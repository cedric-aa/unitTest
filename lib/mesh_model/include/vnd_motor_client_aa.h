/**
 * @file
 * @defgroup Mesh_CB
 * @{
 * @brief API for the Bluetooth Mesh custom/vendor MOTOR.
 */

#ifndef VND_MOTOR_CLIENT_AA_H__
#define VND_MOTOR_CLIENT_AA_H__

#include "vnd_motor_aa.h"

int sendGetIdMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t id, uint8_t seqNum);
int sendSetIdMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t lvl, uint8_t id,
			uint8_t seqNum);
int sendGetAllMotorLevel(struct btMeshMotor *motor, uint16_t addr, uint8_t seqNum);


#endif /* VND_ACTIVATION_AA_H__ */