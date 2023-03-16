#include "model_level_aa.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lvl_srv_AA, LOG_LEVEL_INF);

static void startNewLvlTrans(const struct bt_mesh_lvl_set *set, struct btMeshlevelMotor *ctx)
{
	uint32_t time = set->transition ? set->transition->time : 0;
	uint32_t delay = set->transition ? set->transition->delay : 0;

	ctx->targetLvl = set->lvl;
	ctx->remainingTime = time;
	k_work_reschedule(&ctx->levelMotorDelayWork, K_MSEC(delay));
}

static void updateLvlChange(const struct bt_mesh_lvl_set *set, struct btMeshlevelMotor *ctx)
{
	if (!set->transition) {
		ctx->currentLvl = set->lvl;
		ctx->targetLvl = set->lvl;
	} else {
		startNewLvlTrans(set, ctx);
	}
}

static void levelMotorGet(struct bt_mesh_lvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_lvl_status *rsp)
{
	struct btMeshlevelMotor *levelMotor = CONTAINER_OF(srv, struct btMeshlevelMotor, srvLvl);
	int Motorid = levelMotor - &levelMotors[0];
	LOG_INF("Received [level][GET] op[0x%06x] LevelMotor[%d] from addr:%d revc_addr:%d rssi:%d tid:%d",
		srv->model->op->opcode, Motorid, ctx->addr, ctx->recv_dst, ctx->recv_rssi,
		srv->tid.tid);

	// Set the remaining time for the response to the remaining time of the work delayable plus the remaining time of the context
	rsp->remaining_time = k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(
				      &levelMotor->levelMotorDelayWork)) +
			      levelMotor->remainingTime;

	// Set the current and target levels for the response
	rsp->current = levelMotor->currentLvl;
	rsp->target = levelMotor->targetLvl;
}
static void levelMotorSet(struct bt_mesh_lvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
			  const struct bt_mesh_lvl_set *set, struct bt_mesh_lvl_status *rsp)
{
	struct btMeshlevelMotor *levelMotor = CONTAINER_OF(srv, struct btMeshlevelMotor, srvLvl);
	int Motorid = levelMotor - &levelMotors[0];
	LOG_INF("Received [level][SET] levelMotors[%d] from addr:%d to:%d rssi:%d tid:%d", Motorid,
		ctx->addr, ctx->recv_dst, ctx->recv_rssi, srv->tid.tid);

	if (set->new_transaction) {
		if (set->lvl != levelMotor->targetLvl) {
			updateLvlChange(set, levelMotor);
		}
	}

	rsp->current = levelMotor->currentLvl;
	rsp->target = levelMotor->targetLvl;
	rsp->remaining_time = k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(
				      &levelMotor->levelMotorDelayWork)) +
			      levelMotor->remainingTime;
}
static void levelMotorDelta(struct bt_mesh_lvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
			    const struct bt_mesh_lvl_delta_set *delta_set,
			    struct bt_mesh_lvl_status *rsp)
{
	LOG_DBG("level delta");
}
static void levelMotorMove(struct bt_mesh_lvl_srv *srv, struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_lvl_move_set *move_set,
			   struct bt_mesh_lvl_status *rsp)
{
	LOG_DBG("level move");
}

const struct bt_mesh_lvl_srv_handlers levelMotorHandlers = { .set = levelMotorSet,
							     .get = levelMotorGet,
							     .move_set = levelMotorMove,
							     .delta_set = levelMotorDelta };

void levelMotorWork(struct k_work *work)
{
	int err;

	struct btMeshlevelMotor *levelMotor =
		CONTAINER_OF(work, struct btMeshlevelMotor, levelMotorDelayWork);

	if (levelMotor->remainingTime) {
		k_work_reschedule(&levelMotor->levelMotorDelayWork,
				  K_MSEC(levelMotor->remainingTime));
		levelMotor->remainingTime = 0;
	} else {
		/* Publish the new value at the end of the transition */
		struct bt_mesh_lvl_status status;

		// Update current level to target level and set status values
		levelMotor->currentLvl = levelMotor->targetLvl;
		status.current = levelMotor->currentLvl;
		status.target = levelMotor->targetLvl;
		status.remaining_time = levelMotor->remainingTime;

		// Publish the new values
		err = bt_mesh_lvl_srv_pub(&levelMotor->srvLvl, NULL, &status);
		LOG_INF("Send [level][STATUS] op[0x%06x] from addr:0x%04x.to 0x%04x.   tid:-- ",
			BT_MESH_LVL_OP_STATUS, bt_mesh_model_elem(levelMotor->srvLvl.model)->addr,
			levelMotor->srvLvl.pub.addr);

		// Log error if failed to publish
		if (err) {
			LOG_ERR("error:[%d]", err);
		}
	}
}
