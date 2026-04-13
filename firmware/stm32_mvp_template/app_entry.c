/*
 * HAL-ready app entry for STM32F103 (CubeMX/CubeIDE).
 *
 * Default pin mapping:
 * - PB12 -> AIN1
 * - PB13 -> AIN2
 * - PB14 -> STBY
 * - PA8  -> PWMA (TIM1_CH1)
 * - PA0  -> left limit (active-low)
 * - PA1  -> right limit (active-low)
 * - PA4  -> key open (active-low)
 * - PA5  -> key stop (active-low)
 * - PA6  -> key close (active-low)
 */

#include "app_entry.h"
#include "curtain_ctrl.h"
#include "curtain_port.h"
#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>

static curtain_ctrl_t g_ctx;

extern TIM_HandleTypeDef htim1;
extern UART_HandleTypeDef huart1;

#define MOTOR_AIN1_GPIO_Port GPIOB
#define MOTOR_AIN1_Pin GPIO_PIN_12
#define MOTOR_AIN2_GPIO_Port GPIOB
#define MOTOR_AIN2_Pin GPIO_PIN_13
#define MOTOR_STBY_GPIO_Port GPIOB
#define MOTOR_STBY_Pin GPIO_PIN_14

#define KEY_OPEN_GPIO_Port GPIOA
#define KEY_OPEN_Pin GPIO_PIN_4
#define KEY_STOP_GPIO_Port GPIOA
#define KEY_STOP_Pin GPIO_PIN_5
#define KEY_CLOSE_GPIO_Port GPIOA
#define KEY_CLOSE_Pin GPIO_PIN_6

#define LIMIT_LEFT_GPIO_Port GPIOA
#define LIMIT_LEFT_Pin GPIO_PIN_0
#define LIMIT_RIGHT_GPIO_Port GPIOA
#define LIMIT_RIGHT_Pin GPIO_PIN_1

static uint8_t is_active_low(GPIO_TypeDef *port, uint16_t pin)
{
    return (uint8_t)(HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET);
}

static uint8_t clamp_pwm(uint8_t pwm_percent)
{
    return (pwm_percent > 100u) ? 100u : pwm_percent;
}

void Port_MotorSet(motor_dir_t dir, uint8_t pwm_percent)
{
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
    uint32_t pulse;
    pwm_percent = clamp_pwm(pwm_percent);
    pulse = ((arr + 1u) * (uint32_t)pwm_percent) / 100u;
    if (pulse > arr) {
        pulse = arr;
    }

    switch (dir) {
    case MOTOR_OPEN_DIR:
        HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
        break;
    case MOTOR_CLOSE_DIR:
        HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, GPIO_PIN_SET);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
        break;
    case MOTOR_STOP:
    default:
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        HAL_GPIO_WritePin(MOTOR_AIN1_GPIO_Port, MOTOR_AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_AIN2_GPIO_Port, MOTOR_AIN2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_STBY_GPIO_Port, MOTOR_STBY_Pin, GPIO_PIN_RESET);
        break;
    }
}

void Port_Log(const char *msg)
{
    char line[128];
    int n = snprintf(line, sizeof(line), "%s\r\n", msg);
    if (n > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)strlen(line), 100);
    }
}

void Port_ReadInputs(curtain_inputs_t *in)
{
    static uint8_t prev_open = 0;
    static uint8_t prev_stop = 0;
    static uint8_t prev_close = 0;

    uint8_t cur_open = is_active_low(KEY_OPEN_GPIO_Port, KEY_OPEN_Pin);
    uint8_t cur_stop = is_active_low(KEY_STOP_GPIO_Port, KEY_STOP_Pin);
    uint8_t cur_close = is_active_low(KEY_CLOSE_GPIO_Port, KEY_CLOSE_Pin);

    in->key_open_pressed = (uint8_t)(cur_open && !prev_open);
    in->key_stop_pressed = (uint8_t)(cur_stop && !prev_stop);
    in->key_close_pressed = (uint8_t)(cur_close && !prev_close);

    prev_open = cur_open;
    prev_stop = cur_stop;
    prev_close = cur_close;

    in->left_limit_triggered = is_active_low(LIMIT_LEFT_GPIO_Port, LIMIT_LEFT_Pin);
    in->right_limit_triggered = is_active_low(LIMIT_RIGHT_GPIO_Port, LIMIT_RIGHT_Pin);
}

void App_Init(void)
{
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    Curtain_Init(&g_ctx);
    Port_Log("boot ok");
}

static void App_Loop10ms(void)
{
    Curtain_Tick10ms(&g_ctx);
}

void App_Run10msScheduler(void)
{
    static uint32_t last = 0;
    uint32_t now = HAL_GetTick();
    if ((now - last) >= 10u) {
        last = now;
        App_Loop10ms();
    }
}

