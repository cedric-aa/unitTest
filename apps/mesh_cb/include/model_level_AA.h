/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Model of the levelMotor
 */

#ifndef MODEL_LEVEL_AA_H__
#define MODEL_LEVEL_AA_H__

#include <bluetooth/mesh/gen_lvl_srv.h>

struct btMeshlevelMotor {
	struct bt_mesh_lvl_srv srvLvl;
	struct k_work_delayable levelMotorDelayWork;
	uint32_t timePeriod;
	uint32_t remainingTime;
	uint16_t currentLvl;
	uint16_t targetLvl;
};

void levelMotorWork(struct k_work *work);
extern struct btMeshlevelMotor levelMotors[];
extern const struct bt_mesh_lvl_srv_handlers levelMotorHandlers;

#endif /* MODEL_LEVEL_AA_H__ */
