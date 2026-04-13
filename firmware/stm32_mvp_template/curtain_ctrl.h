#ifndef CURTAIN_CTRL_H
#define CURTAIN_CTRL_H

#include <stdint.h>

typedef enum {
    CURTAIN_IDLE = 0,
    CURTAIN_OPENING,
    CURTAIN_CLOSING,
    CURTAIN_FAULT,
} curtain_state_t;

typedef enum {
    FAULT_NONE = 0x00,
    FAULT_WRONG_LIMIT = 0x01,
    FAULT_TIMEOUT = 0x02,
} fault_code_t;

typedef struct {
    curtain_state_t state;
    fault_code_t fault;
    uint16_t move_ticks_10ms;
    uint16_t timeout_ticks_10ms;
    uint8_t pwm_open;
    uint8_t pwm_close;
} curtain_ctrl_t;

void Curtain_Init(curtain_ctrl_t *ctx);
void Curtain_Tick10ms(curtain_ctrl_t *ctx);
void Curtain_ResetFault(curtain_ctrl_t *ctx);

/*
 * Bring-up configuration:
 * - CURTAIN_USE_LIMIT_SWITCH=0: ignore limit inputs, rely on timeout + manual stop.
 * - CURTAIN_TIMEOUT_AS_FAULT=0: timeout only stops motor, does not latch fault.
 */
#ifndef CURTAIN_USE_LIMIT_SWITCH
#define CURTAIN_USE_LIMIT_SWITCH 0
#endif

#ifndef CURTAIN_TIMEOUT_AS_FAULT
#define CURTAIN_TIMEOUT_AS_FAULT 0
#endif

#ifndef CURTAIN_TIMEOUT_TICKS_10MS
#define CURTAIN_TIMEOUT_TICKS_10MS 1200u
#endif

#endif // CURTAIN_CTRL_H
