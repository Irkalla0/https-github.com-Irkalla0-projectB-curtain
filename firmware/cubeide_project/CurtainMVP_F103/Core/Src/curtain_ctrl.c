#include "curtain_ctrl.h"

#include <stdio.h>

static uint8_t clamp_u8(uint8_t value, uint8_t min_v, uint8_t max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static uint16_t clamp_u16(uint16_t value, uint16_t min_v, uint16_t max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

const char *Curtain_StateName(curtain_state_t state)
{
    switch (state) {
    case CURTAIN_IDLE:
        return "IDLE";
    case CURTAIN_OPENING:
        return "OPENING";
    case CURTAIN_CLOSING:
        return "CLOSING";
    case CURTAIN_STOPPING:
        return "STOPPING";
    case CURTAIN_CALIBRATING:
        return "CALIBRATING";
    case CURTAIN_FAULT_LOCKED:
        return "FAULT_LOCKED";
    default:
        return "UNKNOWN";
    }
}

static void refresh_current_pos(curtain_ctrl_t *ctx)
{
    if (ctx->pos_permille > 1000u) {
        ctx->pos_permille = 1000u;
    }
    ctx->current_pos = (uint8_t)(ctx->pos_permille / 10u);
}

static void apply_pwm_ramp(curtain_ctrl_t *ctx)
{
    if (ctx->pwm_now < ctx->pwm_target) {
        uint16_t next = (uint16_t)ctx->pwm_now + CURTAIN_PWM_RAMP_STEP;
        if (next > ctx->pwm_target) {
            next = ctx->pwm_target;
        }
        ctx->pwm_now = (uint8_t)next;
    } else if (ctx->pwm_now > ctx->pwm_target) {
        if (ctx->pwm_now > CURTAIN_PWM_RAMP_STEP) {
            ctx->pwm_now = (uint8_t)(ctx->pwm_now - CURTAIN_PWM_RAMP_STEP);
        } else {
            ctx->pwm_now = 0u;
        }
        if (ctx->pwm_now < ctx->pwm_target) {
            ctx->pwm_now = ctx->pwm_target;
        }
    }
}

static void apply_motor_output(const curtain_ctrl_t *ctx)
{
    if (ctx->motion_dir == MOTOR_STOP || ctx->pwm_now == 0u) {
        Port_MotorSet(MOTOR_STOP, 0u);
        return;
    }
    Port_MotorSet(ctx->motion_dir, ctx->pwm_now);
}

static void log_simple(const char *msg)
{
    Port_Log(msg);
}

static void enter_idle(curtain_ctrl_t *ctx, const char *reason)
{
    ctx->state = CURTAIN_IDLE;
    ctx->motion_dir = MOTOR_STOP;
    ctx->motion_ticks = 0u;
    ctx->stop_hold_ticks = 0u;
    ctx->pwm_target = 0u;
    ctx->pending_cmd.valid = 0u;
    log_simple(reason);
}

static void enter_stopping(curtain_ctrl_t *ctx, const char *reason)
{
    ctx->state = CURTAIN_STOPPING;
    ctx->motion_dir = MOTOR_STOP;
    ctx->motion_ticks = 0u;
    ctx->stop_hold_ticks = CURTAIN_STOP_HOLD_TICKS_10MS;
    ctx->pwm_target = 0u;
    log_simple(reason);
}

void Curtain_ForceFault(curtain_ctrl_t *ctx, fault_code_t code, const char *reason)
{
    ctx->state = CURTAIN_FAULT_LOCKED;
    ctx->fault_code = code;
    ctx->recoverable = 1u;
    ctx->motion_dir = MOTOR_STOP;
    ctx->motion_ticks = 0u;
    ctx->pwm_target = 0u;
    ctx->stop_hold_ticks = CURTAIN_STOP_HOLD_TICKS_10MS;
    log_simple(reason);
}

static void start_motion(curtain_ctrl_t *ctx, motor_dir_t dir, uint8_t target_pos, const char *reason)
{
    ctx->state = (dir == MOTOR_OPEN_DIR) ? CURTAIN_OPENING : CURTAIN_CLOSING;
    ctx->motion_dir = dir;
    ctx->target_pos = clamp_u8(target_pos, 0u, 100u);
    ctx->motion_ticks = 0u;
    ctx->pwm_target = CURTAIN_PWM_MAX;
    if (ctx->pwm_now < CURTAIN_PWM_MIN) {
        ctx->pwm_now = CURTAIN_PWM_MIN;
    }
    log_simple(reason);
}

static void update_position_estimate(curtain_ctrl_t *ctx)
{
    if (ctx->travel_ticks_full == 0u) {
        ctx->travel_ticks_full = CURTAIN_TRAVEL_TICKS_FULL_DEFAULT;
    }

    ctx->pos_accum = (uint16_t)(ctx->pos_accum + 1000u);
    while (ctx->pos_accum >= ctx->travel_ticks_full) {
        ctx->pos_accum = (uint16_t)(ctx->pos_accum - ctx->travel_ticks_full);
        if (ctx->motion_dir == MOTOR_OPEN_DIR) {
            if (ctx->pos_permille < 1000u) {
                ctx->pos_permille++;
            }
        } else if (ctx->motion_dir == MOTOR_CLOSE_DIR) {
            if (ctx->pos_permille > 0u) {
                ctx->pos_permille--;
            }
        }
    }
    refresh_current_pos(ctx);
}

static uint8_t reached_target(const curtain_ctrl_t *ctx)
{
    uint16_t target_permille = (uint16_t)ctx->target_pos * 10u;
    if (ctx->motion_dir == MOTOR_OPEN_DIR) {
        return (uint8_t)(ctx->pos_permille >= target_permille);
    }
    if (ctx->motion_dir == MOTOR_CLOSE_DIR) {
        return (uint8_t)(ctx->pos_permille <= target_permille);
    }
    return 1u;
}

void Curtain_Init(curtain_ctrl_t *ctx, uint8_t persisted_pos, uint16_t persisted_travel_ticks)
{
    ctx->state = CURTAIN_IDLE;
    ctx->fault_code = FAULT_NONE;
    ctx->recoverable = 1u;
    ctx->current_pos = clamp_u8(persisted_pos, 0u, 100u);
    ctx->target_pos = ctx->current_pos;
    ctx->motion_dir = MOTOR_STOP;
    ctx->pwm_now = 0u;
    ctx->pwm_target = 0u;
    ctx->travel_ticks_full = clamp_u16(persisted_travel_ticks, 200u, 3000u);
    ctx->timeout_ticks = CURTAIN_TIMEOUT_TICKS_10MS;
    ctx->motion_ticks = 0u;
    ctx->stop_hold_ticks = 0u;
    ctx->calibrate_ticks = 0u;
    ctx->calibrate_open_ticks = 0u;
    ctx->calibrate_close_ticks = 0u;
    ctx->pos_permille = (uint16_t)ctx->current_pos * 10u;
    ctx->pos_accum = 0u;
    ctx->calibrate_phase = 0u;
    ctx->last_seq = 0u;
    ctx->pending_cmd.kind = CURTAIN_CMD_NONE;
    ctx->pending_cmd.target_pos = ctx->current_pos;
    ctx->pending_cmd.seq = 0u;
    ctx->pending_cmd.valid = 0u;
    Port_MotorSet(MOTOR_STOP, 0u);
    Port_Log("curtain init -> IDLE");
}

void Curtain_ResetFault(curtain_ctrl_t *ctx)
{
    ctx->fault_code = FAULT_NONE;
    ctx->recoverable = 1u;
    enter_idle(ctx, "fault reset -> IDLE");
}

void Curtain_SubmitCommand(curtain_ctrl_t *ctx, const curtain_command_t *cmd)
{
    if (cmd == 0 || cmd->valid == 0u) {
        return;
    }
    ctx->pending_cmd = *cmd;
}

static void consume_command(curtain_ctrl_t *ctx, curtain_command_t *cmd)
{
    if (ctx->pending_cmd.valid) {
        *cmd = ctx->pending_cmd;
        ctx->pending_cmd.valid = 0u;
        return;
    }
    cmd->kind = CURTAIN_CMD_NONE;
    cmd->target_pos = 0u;
    cmd->seq = 0u;
    cmd->valid = 0u;
}

static void command_from_keys(const curtain_inputs_t *in, curtain_command_t *cmd)
{
    if (in->key_stop_pressed) {
        cmd->kind = CURTAIN_CMD_STOP;
        cmd->target_pos = 0u;
        cmd->seq = 0u;
        cmd->valid = 1u;
        return;
    }
    if (in->key_open_pressed) {
        cmd->kind = CURTAIN_CMD_OPEN;
        cmd->target_pos = 100u;
        cmd->seq = 0u;
        cmd->valid = 1u;
        return;
    }
    if (in->key_close_pressed) {
        cmd->kind = CURTAIN_CMD_CLOSE;
        cmd->target_pos = 0u;
        cmd->seq = 0u;
        cmd->valid = 1u;
        return;
    }
}

static void apply_command(curtain_ctrl_t *ctx, const curtain_command_t *cmd)
{
    uint8_t target;
    if (cmd->valid == 0u) {
        return;
    }

    if (cmd->seq > 0u) {
        ctx->last_seq = cmd->seq;
    }

    if (ctx->state == CURTAIN_FAULT_LOCKED) {
        if (cmd->kind == CURTAIN_CMD_RESET_FAULT) {
            Curtain_ResetFault(ctx);
        }
        return;
    }

    switch (cmd->kind) {
    case CURTAIN_CMD_OPEN:
        start_motion(ctx, MOTOR_OPEN_DIR, 100u, "command -> OPENING");
        break;
    case CURTAIN_CMD_CLOSE:
        start_motion(ctx, MOTOR_CLOSE_DIR, 0u, "command -> CLOSING");
        break;
    case CURTAIN_CMD_STOP:
        enter_stopping(ctx, "command -> STOPPING");
        break;
    case CURTAIN_CMD_SET_TARGET:
        target = clamp_u8(cmd->target_pos, 0u, 100u);
        ctx->target_pos = target;
        if (ctx->current_pos < target) {
            start_motion(ctx, MOTOR_OPEN_DIR, target, "target -> OPENING");
        } else if (ctx->current_pos > target) {
            start_motion(ctx, MOTOR_CLOSE_DIR, target, "target -> CLOSING");
        } else {
            enter_idle(ctx, "target reached -> IDLE");
        }
        break;
    case CURTAIN_CMD_CALIBRATE:
        ctx->state = CURTAIN_CALIBRATING;
        ctx->calibrate_phase = 0u;
        ctx->calibrate_ticks = 0u;
        ctx->calibrate_open_ticks = 0u;
        ctx->calibrate_close_ticks = 0u;
        ctx->motion_dir = MOTOR_OPEN_DIR;
        ctx->pwm_target = CURTAIN_PWM_MAX;
        if (ctx->pwm_now < CURTAIN_PWM_MIN) {
            ctx->pwm_now = CURTAIN_PWM_MIN;
        }
        ctx->motion_ticks = 0u;
        log_simple("command -> CALIBRATING");
        break;
    case CURTAIN_CMD_RESET_FAULT:
        Curtain_ResetFault(ctx);
        break;
    case CURTAIN_CMD_NONE:
    default:
        break;
    }
}

static void safety_checks(curtain_ctrl_t *ctx, const curtain_inputs_t *in)
{
    if (ctx->state == CURTAIN_OPENING || ctx->state == CURTAIN_CLOSING || ctx->state == CURTAIN_CALIBRATING) {
        if (in->pinch_triggered) {
            Curtain_ForceFault(ctx, FAULT_PINCH, "safety fault -> PINCH");
            return;
        }
        if (in->overcurrent_triggered) {
            Curtain_ForceFault(ctx, FAULT_OVERCURRENT, "safety fault -> OVERCURRENT");
            return;
        }
        if (in->jam_triggered) {
            Curtain_ForceFault(ctx, FAULT_JAM, "safety fault -> JAM");
            return;
        }
    }

    if (ctx->state == CURTAIN_OPENING) {
        if (in->left_limit_triggered) {
            Curtain_ForceFault(ctx, FAULT_WRONG_LIMIT, "opening wrong limit -> fault");
            return;
        }
    } else if (ctx->state == CURTAIN_CLOSING) {
        if (in->right_limit_triggered) {
            Curtain_ForceFault(ctx, FAULT_WRONG_LIMIT, "closing wrong limit -> fault");
            return;
        }
    }
}

static void run_calibration(curtain_ctrl_t *ctx, const curtain_inputs_t *in)
{
    ctx->motion_ticks++;
    ctx->calibrate_ticks++;

    if (ctx->calibrate_phase == 0u) {
        ctx->calibrate_open_ticks++;
        update_position_estimate(ctx);

        if (in->right_limit_triggered) {
            ctx->calibrate_phase = 1u;
            ctx->motion_ticks = 0u;
            ctx->motion_dir = MOTOR_CLOSE_DIR;
            ctx->target_pos = 0u;
            ctx->pos_permille = 1000u;
            refresh_current_pos(ctx);
            log_simple("calibration phase open done -> closing");
            return;
        }
        if (in->left_limit_triggered) {
            Curtain_ForceFault(ctx, FAULT_WRONG_LIMIT, "calibration wrong limit -> fault");
            return;
        }
    } else {
        ctx->calibrate_close_ticks++;
        update_position_estimate(ctx);

        if (in->left_limit_triggered) {
            if (ctx->calibrate_close_ticks < 200u) {
                Curtain_ForceFault(ctx, FAULT_CALIBRATION, "calibration invalid travel -> fault");
                return;
            }
            ctx->travel_ticks_full = ctx->calibrate_close_ticks;
            ctx->pos_permille = 0u;
            refresh_current_pos(ctx);
            ctx->target_pos = 0u;
            enter_stopping(ctx, "calibration done -> STOPPING");
            return;
        }
        if (in->right_limit_triggered) {
            Curtain_ForceFault(ctx, FAULT_WRONG_LIMIT, "calibration wrong limit -> fault");
            return;
        }
    }

    if (ctx->motion_ticks > ctx->timeout_ticks) {
        Curtain_ForceFault(ctx, FAULT_CALIBRATION, "calibration timeout -> fault");
    }
}

static void run_motion(curtain_ctrl_t *ctx, const curtain_inputs_t *in)
{
    ctx->motion_ticks++;
    update_position_estimate(ctx);

    if (ctx->state == CURTAIN_OPENING && in->right_limit_triggered) {
        ctx->pos_permille = 1000u;
        refresh_current_pos(ctx);
        enter_stopping(ctx, "open limit reached -> STOPPING");
        return;
    }
    if (ctx->state == CURTAIN_CLOSING && in->left_limit_triggered) {
        ctx->pos_permille = 0u;
        refresh_current_pos(ctx);
        enter_stopping(ctx, "close limit reached -> STOPPING");
        return;
    }

    if (reached_target(ctx)) {
        enter_stopping(ctx, "target reached -> STOPPING");
        return;
    }

    if (ctx->motion_ticks > ctx->timeout_ticks) {
        Curtain_ForceFault(ctx, FAULT_TIMEOUT, "motion timeout -> fault");
    }
}

void Curtain_GetStatus(const curtain_ctrl_t *ctx, curtain_status_t *out)
{
    if (out == 0) {
        return;
    }
    out->state = ctx->state;
    out->current_pos = ctx->current_pos;
    out->target_pos = ctx->target_pos;
    out->fault_code = ctx->fault_code;
    out->recoverable = ctx->recoverable;
    out->last_seq = ctx->last_seq;
}

void Curtain_Tick10ms(curtain_ctrl_t *ctx, const curtain_inputs_t *in)
{
    curtain_command_t cmd;

    if (ctx == 0 || in == 0) {
        return;
    }

    // 1) parse command (external UART first, then local keys)
    consume_command(ctx, &cmd);
    if (cmd.valid == 0u) {
        command_from_keys(in, &cmd);
    }

    // 2) explicit fault reset is allowed from stop key
    if (ctx->state == CURTAIN_FAULT_LOCKED && in->key_stop_pressed) {
        Curtain_ResetFault(ctx);
    }

    // 3) apply command
    apply_command(ctx, &cmd);

    // 4) safety checks
    safety_checks(ctx, in);

    // 5) trajectory update
    if (ctx->state == CURTAIN_CALIBRATING) {
        run_calibration(ctx, in);
    } else if (ctx->state == CURTAIN_OPENING || ctx->state == CURTAIN_CLOSING) {
        run_motion(ctx, in);
    } else if (ctx->state == CURTAIN_STOPPING) {
        if (ctx->stop_hold_ticks > 0u) {
            ctx->stop_hold_ticks--;
        } else if (ctx->pwm_now == 0u) {
            enter_idle(ctx, "stopping complete -> IDLE");
        }
    }

    // 6) output update
    apply_pwm_ramp(ctx);
    apply_motor_output(ctx);
    refresh_current_pos(ctx);
}
