#ifndef CURTAIN_PORT_H
#define CURTAIN_PORT_H

#include <stdint.h>

typedef enum {
    MOTOR_STOP = 0,
    MOTOR_OPEN_DIR = 1,
    MOTOR_CLOSE_DIR = 2,
} motor_dir_t;

typedef struct {
    uint8_t key_open_pressed;
    uint8_t key_stop_pressed;
    uint8_t key_close_pressed;
    uint8_t left_limit_triggered;
    uint8_t right_limit_triggered;
} curtain_inputs_t;

// Hardware abstraction functions (implement these in your Cube project).
void Port_MotorSet(motor_dir_t dir, uint8_t pwm_percent);
void Port_Log(const char *msg);
void Port_ReadInputs(curtain_inputs_t *in);

#endif // CURTAIN_PORT_H

