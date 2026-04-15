#ifndef CURTAIN_CTRL_H
#define CURTAIN_CTRL_H

#include <stdint.h>
#include "curtain_port.h"

typedef enum {
    CURTAIN_IDLE = 0,
    CURTAIN_OPENING,
    CURTAIN_CLOSING,
    CURTAIN_STOPPING,
    CURTAIN_CALIBRATING,
    CURTAIN_FAULT_LOCKED,
} curtain_state_t;

typedef enum {
    FAULT_NONE = 0x00,
    FAULT_WRONG_LIMIT = 0x01,
    FAULT_TIMEOUT = 0x02,
    FAULT_JAM = 0x03,
    FAULT_PINCH = 0x04,
    FAULT_OVERCURRENT = 0x05,
    FAULT_AUTH_FAILED = 0x06,
    FAULT_REPLAY_ATTACK = 0x07,
    FAULT_CALIBRATION = 0x08,
} fault_code_t;

typedef enum {
    CURTAIN_CMD_NONE = 0,
    CURTAIN_CMD_OPEN,
    CURTAIN_CMD_CLOSE,
    CURTAIN_CMD_STOP,
    CURTAIN_CMD_SET_TARGET,
    CURTAIN_CMD_CALIBRATE,
    CURTAIN_CMD_RESET_FAULT,
} curtain_cmd_kind_t;

typedef struct {
    curtain_cmd_kind_t kind;
    uint8_t target_pos;
    uint32_t seq;
    uint8_t valid;
} curtain_command_t;

typedef struct {
    curtain_state_t state;
    fault_code_t fault_code;
    uint8_t recoverable;
    uint8_t current_pos;
    uint8_t target_pos;
    motor_dir_t motion_dir;
    uint8_t pwm_now;
    uint8_t pwm_target;
    uint16_t travel_ticks_full;
    uint16_t timeout_ticks;
    uint16_t motion_ticks;
    uint16_t stop_hold_ticks;
    uint16_t calibrate_ticks;
    uint16_t calibrate_open_ticks;
    uint16_t calibrate_close_ticks;
    uint16_t pos_permille;
    uint16_t pos_accum;
    uint8_t calibrate_phase;
    uint32_t last_seq;
    curtain_command_t pending_cmd;
} curtain_ctrl_t;

typedef struct {
    curtain_state_t state;
    uint8_t current_pos;
    uint8_t target_pos;
    fault_code_t fault_code;
    uint8_t recoverable;
    uint32_t last_seq;
} curtain_status_t;

#ifndef CURTAIN_TIMEOUT_TICKS_10MS
#define CURTAIN_TIMEOUT_TICKS_10MS 1500u
#endif

#ifndef CURTAIN_TRAVEL_TICKS_FULL_DEFAULT
#define CURTAIN_TRAVEL_TICKS_FULL_DEFAULT 900u
#endif

#ifndef CURTAIN_PWM_MIN
#define CURTAIN_PWM_MIN 35u
#endif

#ifndef CURTAIN_PWM_MAX
#define CURTAIN_PWM_MAX 80u
#endif

#ifndef CURTAIN_PWM_RAMP_STEP
#define CURTAIN_PWM_RAMP_STEP 2u
#endif

#ifndef CURTAIN_STOP_HOLD_TICKS_10MS
#define CURTAIN_STOP_HOLD_TICKS_10MS 8u
#endif

void Curtain_Init(curtain_ctrl_t *ctx, uint8_t persisted_pos, uint16_t persisted_travel_ticks);
void Curtain_SubmitCommand(curtain_ctrl_t *ctx, const curtain_command_t *cmd);
void Curtain_ForceFault(curtain_ctrl_t *ctx, fault_code_t code, const char *reason);
void Curtain_ResetFault(curtain_ctrl_t *ctx);
void Curtain_Tick10ms(curtain_ctrl_t *ctx, const curtain_inputs_t *in);
void Curtain_GetStatus(const curtain_ctrl_t *ctx, curtain_status_t *out);
const char *Curtain_StateName(curtain_state_t state);

#endif // CURTAIN_CTRL_H
