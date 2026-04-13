#include "curtain_ctrl.h"
#include "curtain_port.h"

static void enter_idle(curtain_ctrl_t *ctx, const char *log_msg)
{
    ctx->state = CURTAIN_IDLE;
    ctx->move_ticks_10ms = 0;
    Port_MotorSet(MOTOR_STOP, 0);
    Port_Log(log_msg);
}

static void enter_fault(curtain_ctrl_t *ctx, fault_code_t code, const char *log_msg)
{
    ctx->state = CURTAIN_FAULT;
    ctx->fault = code;
    ctx->move_ticks_10ms = 0;
    Port_MotorSet(MOTOR_STOP, 0);
    Port_Log(log_msg);
}

void Curtain_Init(curtain_ctrl_t *ctx)
{
    ctx->state = CURTAIN_IDLE;
    ctx->fault = FAULT_NONE;
    ctx->move_ticks_10ms = 0;
    ctx->timeout_ticks_10ms = CURTAIN_TIMEOUT_TICKS_10MS;
    ctx->pwm_open = 60;
    ctx->pwm_close = 60;
    Port_MotorSet(MOTOR_STOP, 0);
    Port_Log("curtain init -> idle");
}

void Curtain_ResetFault(curtain_ctrl_t *ctx)
{
    ctx->fault = FAULT_NONE;
    enter_idle(ctx, "fault reset -> idle");
}

void Curtain_Tick10ms(curtain_ctrl_t *ctx)
{
    curtain_inputs_t in;
    Port_ReadInputs(&in);

    if (ctx->state == CURTAIN_FAULT) {
        // Example rule: stop key acts as fault reset.
        if (in.key_stop_pressed) {
            Curtain_ResetFault(ctx);
        }
        return;
    }

    // Idle command handling.
    if (ctx->state == CURTAIN_IDLE) {
        if (in.key_open_pressed) {
            ctx->state = CURTAIN_OPENING;
            ctx->move_ticks_10ms = 0;
            Port_MotorSet(MOTOR_OPEN_DIR, ctx->pwm_open);
            Port_Log("idle -> opening");
            return;
        }
        if (in.key_close_pressed) {
            ctx->state = CURTAIN_CLOSING;
            ctx->move_ticks_10ms = 0;
            Port_MotorSet(MOTOR_CLOSE_DIR, ctx->pwm_close);
            Port_Log("idle -> closing");
            return;
        }
        return;
    }

    // Stop key is always valid during motion.
    if (in.key_stop_pressed) {
        enter_idle(ctx, "motion -> idle (manual stop)");
        return;
    }

#if CURTAIN_USE_LIMIT_SWITCH
    // Motion state handling when limit switches are connected.
    if (ctx->state == CURTAIN_OPENING) {
        if (in.right_limit_triggered) {
            enter_idle(ctx, "opening -> idle (right limit)");
            return;
        }
        if (in.left_limit_triggered) {
            enter_fault(ctx, FAULT_WRONG_LIMIT, "opening wrong limit -> fault(0x01)");
            return;
        }
    } else if (ctx->state == CURTAIN_CLOSING) {
        if (in.left_limit_triggered) {
            enter_idle(ctx, "closing -> idle (left limit)");
            return;
        }
        if (in.right_limit_triggered) {
            enter_fault(ctx, FAULT_WRONG_LIMIT, "closing wrong limit -> fault(0x01)");
            return;
        }
    }
#endif

    // Timeout protection.
    ctx->move_ticks_10ms++;
    if (ctx->move_ticks_10ms > ctx->timeout_ticks_10ms) {
#if CURTAIN_TIMEOUT_AS_FAULT
        enter_fault(ctx, FAULT_TIMEOUT, "motion timeout -> fault(0x02)");
#else
        enter_idle(ctx, "motion timeout -> idle (safety stop)");
#endif
    }
}
