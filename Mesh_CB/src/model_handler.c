/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "model_level_aa.h"
#include "vnd_unit_control_server_aa.h"
#include <zephyr/logging/log.h>
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(model_handler, LOG_LEVEL_INF);

#define THREAD_PUBLISHER_STACKSIZE 2048
#define THREAD_PUBLISHER_PRIORITY 14

struct btMeshlevelMotor levelMotors[] = { { .srvLvl = BT_MESH_LVL_SRV_INIT(&levelMotorHandlers) } };

struct btMeshUnitControl unitControl = {
	.handlers = &unitControlHandlers,
};

static struct btMeshActivation activation = {
	.handlers = &activationHandlers,
};

//settings used to store value in the flash
struct settingsControlState settingsCtlState = { .activation = &activation };

struct settingsControlState *const ctl = &settingsCtlState;

void publisherThread(void)
{
	dataQueueItemType publisherQueueItem;

	while (1) {
		k_msgq_get(&publisherQueue, &publisherQueueItem, K_FOREVER);

		LOG_HEXDUMP_INF(publisherQueueItem.bufferItem, publisherQueueItem.length,
				"Cb Publisher Thread");

		switch (publisherQueueItem.bufferItem[0]) {
		case UNIT_CONTROL_TYPE:

			if (publisherQueueItem.bufferItem[1] == SETACK) {
				LOG_INF("reveived uart [status] message from the Cb");
				sendUnitControlFullCmdSetAck(&unitControl, 1);

			} else if (publisherQueueItem.bufferItem[1] == STATUS) {
				LOG_INF("reveived uart [status] message from the Cb");
				unitControlUpdateStatus(&unitControl, publisherQueueItem.bufferItem,
							publisherQueueItem.length);
			}

			break;
		case ACTIVATION_TYPE:
			break;

		default:
			break;
		}
	}
}

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;

	if (attention) {
		dk_set_leds(BIT((idx++) % 2));
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_UNIT_CONTROL(&unitControl),
					BT_MESH_MODEL_ACTIVATION(&activation),
					BT_MESH_MODEL_LVL_SRV(&levelMotors[0].srvLvl))),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	for (int i = 0; i < ARRAY_SIZE(levelMotors); ++i) {
		k_work_init_delayable(&levelMotors[i].levelMotorDelayWork, levelMotorWork);
	}

	return &comp;
}
