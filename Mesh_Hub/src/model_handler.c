#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include "model_handler.h"
#include "model_sensor_cli_aa.h"
#include "vnd_unit_control_client_aa.h"
#include "vnd_motor_aa.h"
#include "vnd_activation_aa.h"
#include "uart_aa.h"
#include "message_format_aa.h"

LOG_MODULE_REGISTER(model_handler, LOG_LEVEL_INF);

#define THREAD_PUBLISHER_STACKSIZE 2048
#define THREAD_PUBLISHER_PRIORITY 14

static struct bt_mesh_sensor_cli btMeshsensorCli = BT_MESH_SENSOR_CLI_INIT(&sensorCliHandlers);

struct btMeshUnitControl unitControl = {
	.handlers = &unitControlHandlers,
};

static struct btMeshMotor motor = {
	.handlers = &motorHandlers,
};
static struct btMeshActivation activation = {
	.handlers = &activationHandlers,
};

struct SettingsControlState settingsCtlState = {
	.activation = &activation,
};

struct SettingsControlState *const ctl = &settingsCtlState;

void publisherThread(void)
{
	dataQueueItemType publisherQueueItem;
	int err;

	while (1) {
		k_msgq_get(&publisherQueue, &publisherQueueItem, K_FOREVER);

		LOG_HEXDUMP_INF(publisherQueueItem.bufferItem, publisherQueueItem.length,
				"Hub Publisher Thread");

		processedMessage processedMessage = processPublisherQueueItem(&publisherQueueItem);

		if (!bt_mesh_is_provisioned()) {
			err = -1;
		} else {
			switch (processedMessage.messageType) {
			case UNIT_CONTROL_TYPE:

				if (processedMessage.messageID == SET) {
					err = sendUnitControlFullCmdSet(
						&unitControl, processedMessage.payloadBuffer,
						processedMessage.payloadLength,
						processedMessage.address);
				} else if (processedMessage.messageID == GET) {
					err = sendUnitControlFullCmdGet(&unitControl,
									processedMessage.address);
				}
				break;
			case ACTIVATION_TYPE:

				if (processedMessage.messageID == SET) {
					err = sendActivationSetPwd(&activation,
								   processedMessage.address,
								   processedMessage.payloadBuffer,
								   processedMessage.payloadLength);
				} else if (processedMessage.messageID == GET) {
					err = sendActivationGetStatus(&activation,
								      processedMessage.address);
				}

				break;
			case SENSOR_TYPE:

				if (processedMessage.messageID == GET) {
					err = bt_mesh_sensor_cli_get(
						&btMeshsensorCli, NULL,
						&bt_mesh_sensor_present_dev_op_temp, NULL);
				}

				break;
			case MOTOR_TYPE:

				if (processedMessage.messageID == SET) {
					err = sendSetIdMotorLevel(
						&motor, processedMessage.address,
						processedMessage.payloadBuffer[0], //id
						processedMessage.payloadBuffer[1], //level
						processedMessage.payloadBuffer[2]); //seqNum

				} else if (processedMessage.messageID == GET_ID) {
					err = sendGetIdMotorLevel(
						&motor, processedMessage.address,
						processedMessage.payloadBuffer[0],
						processedMessage.payloadBuffer[1]);
				} else if (processedMessage.messageID == GET) {
					err = sendGetAllMotorLevel(
						&motor, processedMessage.address,
						processedMessage.payloadBuffer[0]);
				}

				break;

			default:
				break;
			}
		}
		if (processedMessage.isUartAck) {
			// send throug uart the response
			// err code
			// push it to the queue handled by the uart thread
		}
	}
}

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
					BT_MESH_MODEL_ACTIVATION(&activation),
					BT_MESH_MODEL_MOTOR(&motor))),
	BT_MESH_ELEM(2, BT_MESH_MODEL_LIST(BT_MESH_MODEL_SENSOR_CLI(&btMeshsensorCli)),
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

K_THREAD_DEFINE(publisherThread_id, THREAD_PUBLISHER_STACKSIZE, publisherThread, NULL, NULL, NULL,
		THREAD_PUBLISHER_PRIORITY, 0, 0);