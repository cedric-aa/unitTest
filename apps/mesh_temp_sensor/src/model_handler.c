/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include "vnd_sensor_server_aa.h"
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

static struct k_work_delayable updateSensorWork;

static struct btMeshSensor sensor = {
	.handlers = &sensorHandlers,
	.pub.addr = 0xc000,
};

static void updateWork(struct k_work *work)
{
	updateTemp(&sensor);
	k_work_reschedule(&updateSensorWork, K_MSEC(5000));
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (pressed & BIT(0)) {
		printk("Button display sensor");
		updateTemp(&sensor);
	}
	if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER) && (pressed & BIT(3))) {
		printk("Button LPN");
		bt_mesh_proxy_identity_enable();
		return;
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
#if DT_NODE_EXISTS(DT_ALIAS(led0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
		BIT(3),
#endif
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
					BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
					BT_MESH_MODEL_SENSOR(&sensor)),
		     BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);
	k_work_init_delayable(&updateSensorWork, updateWork);
	k_work_schedule(&updateSensorWork, K_MSEC(5000));
	dk_button_handler_add(&button_handler);

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		settings_subsys_init();
	}

	return &comp;
}
