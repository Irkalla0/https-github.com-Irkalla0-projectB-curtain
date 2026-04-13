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

#endif // CURTAIN_CTRL_H

