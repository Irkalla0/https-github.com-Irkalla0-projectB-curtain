/*
 * Example integration snippet for CubeIDE project.
 * Replace placeholder logic with your HAL GPIO/TIM/UART calls.
 */

#include "curtain_ctrl.h"
#include "curtain_port.h"
#include <stdio.h>

static curtain_ctrl_t g_ctx;

// ----------------- Port layer (replace with real hardware logic) -----------------
void Port_MotorSet(motor_dir_t dir, uint8_t pwm_percent)
{
    (void)dir;
    (void)pwm_percent;
    // TODO:
    // 1) Set AIN1/AIN2 according to dir
    // 2) Set PWM duty cycle
    // 3) If dir==MOTOR_STOP then disable drive
}

void Port_Log(const char *msg)
{
    // TODO: send to UART (printf or HAL_UART_Transmit)
    printf("%s\r\n", msg);
}

void Port_ReadInputs(curtain_inputs_t *in)
{
    // TODO: replace with debounced key/limit input reads
    in->key_open_pressed = 0;
    in->key_stop_pressed = 0;
    in->key_close_pressed = 0;
    in->left_limit_triggered = 0;
    in->right_limit_triggered = 0;
}

// ----------------- Example app loop -----------------
void App_Init(void)
{
    Curtain_Init(&g_ctx);
}

void App_Loop10ms(void)
{
    Curtain_Tick10ms(&g_ctx);
}

/*
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_USART1_UART_Init();

    App_Init();

    uint32_t last = HAL_GetTick();
    while (1) {
        uint32_t now = HAL_GetTick();
        if (now - last >= 10) {
            last = now;
            App_Loop10ms();
        }
    }
}
*/

