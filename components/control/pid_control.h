/* pid_control.h – дискретний ПІД-регулятор освітленості */

#pragma once
#include "esp_err.h"

typedef struct
{
    float kp;            /* пропорційний коеф. */
    float ki;            /* інтегральний коеф. */
    float kd;            /* диференційний коеф. */
    float i_min;         /* межі анти-windup (мін.) */
    float i_max;         /* межі анти-windup (макс.) */
    float out_min;       /* мін. скважність PWM   */
    float out_max;       /* макс. скважність PWM  */
    float dt;            /* період виклику, сек   */
} pid_cfg_t;

/* ініціалізація з конфігурацією зберігає внутрішній стан */
esp_err_t pid_control_init(const pid_cfg_t *cfg);

/* розрахунок скважності: setpoint – лк, meas – лк, → 0…1 */
float pid_control_update(float setpoint, float meas);

/* службове: обнулити інтегральну складову */
void  pid_control_reset(void);
