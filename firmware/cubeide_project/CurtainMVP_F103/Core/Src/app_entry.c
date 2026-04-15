#include "app_entry.h"
#include "curtain_ctrl.h"
#include "curtain_port.h"
#include "stm32f1xx_hal.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static curtain_ctrl_t g_ctx;
static uint16_t g_alive_ticks_10ms = 0;

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

#define CURTAIN_DEVICE_ID "curtain-node-001"
#define CURTAIN_HMAC_KEY "projectb_demo_hmac_key"
#define CURTAIN_REPLAY_WINDOW_SEC 120u

#define UART_LINE_MAX 256

#define CURTAIN_PERSIST_MAGIC 0x43555254u
#define CURTAIN_PERSIST_VERSION 0x0001u
#define CURTAIN_PERSIST_FLASH_ADDR 0x0800FC00u

#ifndef CURTAIN_ENABLE_FLASH_PERSIST
#define CURTAIN_ENABLE_FLASH_PERSIST 1
#endif

typedef struct {
    char device_id[40];
    char cmd[24];
    uint8_t target_pos;
    uint32_t timestamp;
    char nonce[40];
    uint32_t seq;
    char hmac_hex[65];
} signed_frame_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint8_t current_pos;
    uint8_t reserved;
    uint16_t travel_ticks;
    uint32_t crc32;
} persist_data_t;

static char g_uart_rx_line[UART_LINE_MAX];
static uint16_t g_uart_rx_len = 0;
static uint32_t g_last_seq = 0;
static uint32_t g_last_timestamp = 0;
static char g_last_nonce[40] = {0};

static curtain_state_t g_last_state = CURTAIN_IDLE;
static uint8_t g_last_pos = 0;
static fault_code_t g_last_fault = FAULT_NONE;

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
    char line[160];
    int n = snprintf(line, sizeof(line), "# %s\r\n", msg);
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

    // Optional sensors are not wired in MVP hardware by default.
    in->jam_triggered = 0u;
    in->pinch_triggered = 0u;
    in->overcurrent_triggered = 0u;
}

static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    uint32_t i;
    uint8_t bit;
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8u; bit++) {
            if (crc & 1u) {
                crc = (crc >> 1u) ^ 0xEDB88320u;
            } else {
                crc >>= 1u;
            }
        }
    }
    return ~crc;
}

#if CURTAIN_ENABLE_FLASH_PERSIST
static void persist_load(uint8_t *current_pos, uint16_t *travel_ticks)
{
    const persist_data_t *raw = (const persist_data_t *)CURTAIN_PERSIST_FLASH_ADDR;
    uint32_t crc;

    *current_pos = 50u;
    *travel_ticks = CURTAIN_TRAVEL_TICKS_FULL_DEFAULT;

    if (raw->magic != CURTAIN_PERSIST_MAGIC || raw->version != CURTAIN_PERSIST_VERSION) {
        return;
    }

    crc = crc32_calc((const uint8_t *)raw, sizeof(persist_data_t) - sizeof(uint32_t));
    if (crc != raw->crc32) {
        return;
    }

    *current_pos = raw->current_pos;
    *travel_ticks = raw->travel_ticks;
}

static void persist_save(uint8_t current_pos, uint16_t travel_ticks)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t page_error = 0;
    persist_data_t data;
    const uint16_t *half = (const uint16_t *)&data;
    uint32_t i;

    data.magic = CURTAIN_PERSIST_MAGIC;
    data.version = CURTAIN_PERSIST_VERSION;
    data.current_pos = current_pos;
    data.reserved = 0u;
    data.travel_ticks = travel_ticks;
    data.crc32 = crc32_calc((const uint8_t *)&data, sizeof(persist_data_t) - sizeof(uint32_t));

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = CURTAIN_PERSIST_FLASH_ADDR;
    erase.NbPages = 1;
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return;
    }

    for (i = 0u; i < (sizeof(persist_data_t) / 2u); i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, CURTAIN_PERSIST_FLASH_ADDR + (i * 2u), half[i]) != HAL_OK) {
            break;
        }
    }

    HAL_FLASH_Lock();
}
#else
static void persist_load(uint8_t *current_pos, uint16_t *travel_ticks)
{
    *current_pos = 50u;
    *travel_ticks = CURTAIN_TRAVEL_TICKS_FULL_DEFAULT;
}

static void persist_save(uint8_t current_pos, uint16_t travel_ticks)
{
    (void)current_pos;
    (void)travel_ticks;
}
#endif

// ---- SHA256 + HMAC-SHA256 ----
typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} sha256_ctx_t;

static const uint32_t k256[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u,
};

#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32u - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT((x), 2u) ^ ROTRIGHT((x), 13u) ^ ROTRIGHT((x), 22u))
#define EP1(x) (ROTRIGHT((x), 6u) ^ ROTRIGHT((x), 11u) ^ ROTRIGHT((x), 25u))
#define SIG0(x) (ROTRIGHT((x), 7u) ^ ROTRIGHT((x), 18u) ^ ((x) >> 3u))
#define SIG1(x) (ROTRIGHT((x), 17u) ^ ROTRIGHT((x), 19u) ^ ((x) >> 10u))

static void sha256_transform(sha256_ctx_t *ctx, const uint8_t data[])
{
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t i;
    uint32_t m[64];
    uint32_t t1;
    uint32_t t2;

    for (i = 0u; i < 16u; i++) {
        m[i] = ((uint32_t)data[i * 4u] << 24u) | ((uint32_t)data[i * 4u + 1u] << 16u) |
               ((uint32_t)data[i * 4u + 2u] << 8u) | ((uint32_t)data[i * 4u + 3u]);
    }
    for (; i < 64u; i++) {
        m[i] = SIG1(m[i - 2u]) + m[i - 7u] + SIG0(m[i - 15u]) + m[i - 16u];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0u; i < 64u; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + k256[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(sha256_ctx_t *ctx)
{
    ctx->datalen = 0u;
    ctx->bitlen = 0u;
    ctx->state[0] = 0x6a09e667u;
    ctx->state[1] = 0xbb67ae85u;
    ctx->state[2] = 0x3c6ef372u;
    ctx->state[3] = 0xa54ff53au;
    ctx->state[4] = 0x510e527fu;
    ctx->state[5] = 0x9b05688cu;
    ctx->state[6] = 0x1f83d9abu;
    ctx->state[7] = 0x5be0cd19u;
}

static void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    for (i = 0u; i < len; i++) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64u) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512u;
            ctx->datalen = 0u;
        }
    }
}

static void sha256_final(sha256_ctx_t *ctx, uint8_t hash[32])
{
    uint32_t i;
    uint32_t j;

    i = ctx->datalen;
    if (ctx->datalen < 56u) {
        ctx->data[i++] = 0x80u;
        while (i < 56u) {
            ctx->data[i++] = 0u;
        }
    } else {
        ctx->data[i++] = 0x80u;
        while (i < 64u) {
            ctx->data[i++] = 0u;
        }
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56u);
    }

    ctx->bitlen += (uint64_t)ctx->datalen * 8u;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8u);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16u);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24u);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32u);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40u);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48u);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56u);
    sha256_transform(ctx, ctx->data);

    for (j = 0u; j < 4u; j++) {
        hash[j] = (uint8_t)((ctx->state[0] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 4u] = (uint8_t)((ctx->state[1] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 8u] = (uint8_t)((ctx->state[2] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 12u] = (uint8_t)((ctx->state[3] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 16u] = (uint8_t)((ctx->state[4] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 20u] = (uint8_t)((ctx->state[5] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 24u] = (uint8_t)((ctx->state[6] >> (24u - j * 8u)) & 0x000000ffu);
        hash[j + 28u] = (uint8_t)((ctx->state[7] >> (24u - j * 8u)) & 0x000000ffu);
    }
}

static void hmac_sha256(const uint8_t *key, uint32_t key_len, const uint8_t *msg, uint32_t msg_len, uint8_t out[32])
{
    uint8_t kopad[64];
    uint8_t kipad[64];
    uint8_t keybuf[64];
    uint8_t tk[32];
    uint32_t i;
    sha256_ctx_t ctx;

    memset(keybuf, 0, sizeof(keybuf));
    if (key_len > 64u) {
        sha256_init(&ctx);
        sha256_update(&ctx, key, key_len);
        sha256_final(&ctx, tk);
        memcpy(keybuf, tk, 32u);
    } else {
        memcpy(keybuf, key, key_len);
    }

    for (i = 0u; i < 64u; i++) {
        kipad[i] = (uint8_t)(keybuf[i] ^ 0x36u);
        kopad[i] = (uint8_t)(keybuf[i] ^ 0x5cu);
    }

    sha256_init(&ctx);
    sha256_update(&ctx, kipad, 64u);
    sha256_update(&ctx, msg, msg_len);
    sha256_final(&ctx, tk);

    sha256_init(&ctx);
    sha256_update(&ctx, kopad, 64u);
    sha256_update(&ctx, tk, 32u);
    sha256_final(&ctx, out);
}

static void bytes_to_hex(const uint8_t *in, uint32_t len, char *out)
{
    static const char *hex = "0123456789abcdef";
    uint32_t i;
    for (i = 0u; i < len; i++) {
        out[i * 2u] = hex[(in[i] >> 4u) & 0x0Fu];
        out[i * 2u + 1u] = hex[in[i] & 0x0Fu];
    }
    out[len * 2u] = '\0';
}

static int str_eq_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if ((char)tolower((unsigned char)*a) != (char)tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

static void send_response_line(uint32_t seq, const char *result, const char *reason)
{
    curtain_status_t st;
    char line[220];
    int n;

    Curtain_GetStatus(&g_ctx, &st);
    n = snprintf(
        line,
        sizeof(line),
        "seq=%lu|result=%s|state=%s|current_pos=%u|fault_code=%u|reason=%s\r\n",
        (unsigned long)seq,
        result,
        Curtain_StateName(st.state),
        st.current_pos,
        (unsigned int)st.fault_code,
        reason);

    if (n > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)strlen(line), 100);
    }
}

static int parse_kv_frame(char *line, signed_frame_t *out)
{
    char *tok;
    char *saveptr = 0;
    uint8_t seen = 0u;

    memset(out, 0, sizeof(*out));
    out->target_pos = 0u;

    tok = strtok_r(line, "|", &saveptr);
    while (tok != 0) {
        char *eq = strchr(tok, '=');
        if (eq != 0) {
            *eq = '\0';
            eq++;
            if (strcmp(tok, "device_id") == 0) {
                strncpy(out->device_id, eq, sizeof(out->device_id) - 1u);
                seen |= 0x01u;
            } else if (strcmp(tok, "cmd") == 0) {
                strncpy(out->cmd, eq, sizeof(out->cmd) - 1u);
                seen |= 0x02u;
            } else if (strcmp(tok, "target_pos") == 0) {
                long v = strtol(eq, 0, 10);
                if (v < 0) {
                    v = 0;
                }
                if (v > 100) {
                    v = 100;
                }
                out->target_pos = (uint8_t)v;
                seen |= 0x04u;
            } else if (strcmp(tok, "timestamp") == 0) {
                out->timestamp = (uint32_t)strtoul(eq, 0, 10);
                seen |= 0x08u;
            } else if (strcmp(tok, "nonce") == 0) {
                strncpy(out->nonce, eq, sizeof(out->nonce) - 1u);
                seen |= 0x10u;
            } else if (strcmp(tok, "seq") == 0) {
                out->seq = (uint32_t)strtoul(eq, 0, 10);
                seen |= 0x20u;
            } else if (strcmp(tok, "hmac") == 0) {
                strncpy(out->hmac_hex, eq, sizeof(out->hmac_hex) - 1u);
                seen |= 0x40u;
            }
        }
        tok = strtok_r(0, "|", &saveptr);
    }

    return ((seen & 0x7Fu) == 0x7Fu);
}

static int verify_frame_hmac(const signed_frame_t *in)
{
    char canonical[220];
    uint8_t hash[32];
    char hash_hex[65];
    int n;

    n = snprintf(
        canonical,
        sizeof(canonical),
        "device_id=%s|cmd=%s|target_pos=%u|timestamp=%lu|nonce=%s|seq=%lu",
        in->device_id,
        in->cmd,
        (unsigned int)in->target_pos,
        (unsigned long)in->timestamp,
        in->nonce,
        (unsigned long)in->seq);
    if (n <= 0) {
        return 0;
    }

    hmac_sha256(
        (const uint8_t *)CURTAIN_HMAC_KEY,
        (uint32_t)strlen(CURTAIN_HMAC_KEY),
        (const uint8_t *)canonical,
        (uint32_t)strlen(canonical),
        hash);
    bytes_to_hex(hash, 32u, hash_hex);

    return str_eq_ci(hash_hex, in->hmac_hex);
}

static int map_cmd(const char *cmd, curtain_command_t *out, uint8_t target_pos)
{
    out->kind = CURTAIN_CMD_NONE;
    out->target_pos = target_pos;
    out->valid = 1u;

    if (strcmp(cmd, "target") == 0) {
        out->kind = CURTAIN_CMD_SET_TARGET;
        return 1;
    }
    if (strcmp(cmd, "open") == 0) {
        out->kind = CURTAIN_CMD_OPEN;
        out->target_pos = 100u;
        return 1;
    }
    if (strcmp(cmd, "close") == 0) {
        out->kind = CURTAIN_CMD_CLOSE;
        out->target_pos = 0u;
        return 1;
    }
    if (strcmp(cmd, "stop") == 0) {
        out->kind = CURTAIN_CMD_STOP;
        out->target_pos = 0u;
        return 1;
    }
    if (strcmp(cmd, "calibrate") == 0) {
        out->kind = CURTAIN_CMD_CALIBRATE;
        return 1;
    }
    if (strcmp(cmd, "reset_fault") == 0) {
        out->kind = CURTAIN_CMD_RESET_FAULT;
        return 1;
    }
    return 0;
}

static void handle_signed_command_line(char *line)
{
    signed_frame_t frame;
    curtain_command_t cmd;

    if (!parse_kv_frame(line, &frame)) {
        send_response_line(0u, "reject", "format");
        return;
    }

    if (strcmp(frame.device_id, CURTAIN_DEVICE_ID) != 0) {
        Curtain_ForceFault(&g_ctx, FAULT_AUTH_FAILED, "security fault -> DEVICE");
        send_response_line(frame.seq, "reject", "device_id");
        return;
    }

    if (frame.seq <= g_last_seq || strcmp(frame.nonce, g_last_nonce) == 0) {
        Curtain_ForceFault(&g_ctx, FAULT_REPLAY_ATTACK, "security fault -> REPLAY");
        send_response_line(frame.seq, "reject", "replay");
        return;
    }

    if (g_last_timestamp != 0u) {
        if (frame.timestamp + CURTAIN_REPLAY_WINDOW_SEC < g_last_timestamp) {
            Curtain_ForceFault(&g_ctx, FAULT_REPLAY_ATTACK, "security fault -> STALE");
            send_response_line(frame.seq, "reject", "timestamp");
            return;
        }
    }

    if (!verify_frame_hmac(&frame)) {
        Curtain_ForceFault(&g_ctx, FAULT_AUTH_FAILED, "security fault -> HMAC");
        send_response_line(frame.seq, "reject", "hmac");
        return;
    }

    memset(&cmd, 0, sizeof(cmd));
    if (!map_cmd(frame.cmd, &cmd, frame.target_pos)) {
        send_response_line(frame.seq, "reject", "cmd");
        return;
    }

    cmd.seq = frame.seq;
    Curtain_SubmitCommand(&g_ctx, &cmd);

    g_last_seq = frame.seq;
    g_last_timestamp = frame.timestamp;
    strncpy(g_last_nonce, frame.nonce, sizeof(g_last_nonce) - 1u);

    send_response_line(frame.seq, "ok", "accepted");
}

static void process_uart_rx_nonblocking(void)
{
    uint8_t ch;
    while (HAL_UART_Receive(&huart1, &ch, 1u, 0u) == HAL_OK) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            g_uart_rx_line[g_uart_rx_len] = '\0';
            if (g_uart_rx_len > 0u) {
                handle_signed_command_line(g_uart_rx_line);
            }
            g_uart_rx_len = 0u;
            continue;
        }
        if (g_uart_rx_len < (UART_LINE_MAX - 1u)) {
            g_uart_rx_line[g_uart_rx_len++] = (char)ch;
        } else {
            g_uart_rx_len = 0u;
            send_response_line(0u, "reject", "line_too_long");
        }
    }
}

static void emit_status_if_changed(void)
{
    curtain_status_t st;
    char line[160];
    int n;

    Curtain_GetStatus(&g_ctx, &st);
    if (st.state == g_last_state && st.current_pos == g_last_pos && st.fault_code == g_last_fault) {
        return;
    }

    n = snprintf(
        line,
        sizeof(line),
        "seq=%lu|result=notify|state=%s|current_pos=%u|fault_code=%u|reason=state_change\r\n",
        (unsigned long)st.last_seq,
        Curtain_StateName(st.state),
        st.current_pos,
        (unsigned int)st.fault_code);
    if (n > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)strlen(line), 100);
    }

    g_last_state = st.state;
    g_last_pos = st.current_pos;
    g_last_fault = st.fault_code;
}

void App_Init(void)
{
    uint8_t persisted_pos;
    uint16_t persisted_travel;

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    persist_load(&persisted_pos, &persisted_travel);
    Curtain_Init(&g_ctx, persisted_pos, persisted_travel);

    g_last_state = CURTAIN_IDLE;
    g_last_pos = persisted_pos;
    g_last_fault = FAULT_NONE;

    Port_Log("boot ok");
}

static void App_Loop10ms(void)
{
    curtain_inputs_t in;
    Port_ReadInputs(&in);

    process_uart_rx_nonblocking();
    Curtain_Tick10ms(&g_ctx, &in);
    emit_status_if_changed();
}

void App_Run10msScheduler(void)
{
    static uint32_t last = 0;
    static uint16_t persist_ticks = 0;
    uint32_t now = HAL_GetTick();

    if ((now - last) >= 10u) {
        last = now;
        App_Loop10ms();

        g_alive_ticks_10ms++;
        if (g_alive_ticks_10ms >= 100u) {
            g_alive_ticks_10ms = 0u;
            Port_Log("alive");
        }

        persist_ticks++;
        if (persist_ticks >= 500u) {
            persist_ticks = 0u;
            persist_save(g_ctx.current_pos, g_ctx.travel_ticks_full);
        }
    }
}
