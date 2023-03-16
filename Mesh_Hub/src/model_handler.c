/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include "model_sensor_cli_aa.h"
#include "model_level_cli_aa.h"
#include "vnd_unit_control_aa.h"
#include "vnd_activation_aa.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(model_handler, LOG_LEVEL_INF);

#define GET_DATA_INTERVAL 5000

static struct bt_mesh_lvl_cli btMeshlevelMotorCli = BT_MESH_LVL_CLI_INIT(&levelMotorStatusHandler);

static struct bt_mesh_sensor_cli btMeshsensorCli = BT_MESH_SENSOR_CLI_INIT(&sensorCliHandlers);

static struct btMeshUnitControl unitControl = {
	.handlers = &unitControlHandlers,
};

static struct btMeshActivation activation = {
	.handlers = &activationHandlers,
};

struct SettingsControlState settingsCtlState = {
	.activation = &activation,
};

struct SettingsControlState *const ctl = &settingsCtlState;

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (pressed & BIT(0)) {
		LOG_INF("button1 Test");
	}
	if (pressed & BIT(1)) {
		LOG_INF("button2 Test ");
	}
	if (pressed & BIT(2)) {
		LOG_INF("button3 Test");
	}
	if (pressed & BIT(3)) {
		LOG_INF("button4 Test");
	}
}

static struct button_handler button_handler = {
	.cb = button_handler_cb,
};

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	static int idx;
	const uint8_t pattern[] = {
		BIT(0) | BIT(1),
		BIT(1) | BIT(2),
		BIT(2) | BIT(3),
		BIT(3) | BIT(0),
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
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
					BT_MESH_MODEL_ACTIVATION(&activation))),
	BT_MESH_ELEM(2,
		     BT_MESH_MODEL_LIST(BT_MESH_MODEL_LVL_CLI(&btMeshlevelMotorCli),
					BT_MESH_MODEL_SENSOR_CLI(&btMeshsensorCli)),
		     BT_MESH_MODEL_NONE)
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);
	dk_button_handler_add(&button_handler);

	return &comp;
}
