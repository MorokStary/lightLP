/* pwm_driver.c – MCPWM для ШІМ-димування світильника */

#include "pwm_driver.h"
#include "driver/mcpwm_prelude.h"

#define PWM_GPIO      4
#define PWM_FREQUENCY 1000   /* Гц */

static mcpwm_cmpr_handle_t comparator;
static mcpwm_oper_handle_t oper;

esp_err_t pwm_driver_init(void)
{
    mcpwm_timer_handle_t timer;
    mcpwm_timer_config_t tcfg = {
        .group_id = 0,
        .clk_src  = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 1e6,                /* 1 МГц */
        .period_ticks  = 1000,               /* 1 кГц */
        .count_mode    = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&tcfg, &timer));

    mcpwm_oper_config_t ocfg = { .group_id = 0 };
    ESP_ERROR_CHECK(mcpwm_new_operator(&ocfg, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    mcpwm_comparator_config_t ccfg = { .flags.update_cmp_on_tez = true };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &ccfg, &comparator));

    mcpwm_generator_config_t gcfg = {
        .gen_gpio_num = PWM_GPIO,
        .flags = { .invert_pwm = false }
    };
    mcpwm_gen_handle_t generator;
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gcfg, &generator));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
        generator, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                                MCPWM_TIMER_EVENT_EMPTY,
                                                MCPWM_GEN_ACTION_HIGH)));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
        generator, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP,
                                                  comparator,
                                                  MCPWM_GEN_ACTION_LOW)));

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    return pwm_driver_set_duty(0.0f);
}

esp_err_t pwm_driver_set_duty(float duty)
{
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
    uint32_t ticks = (uint32_t)(duty * 1000.0f);          /* period_ticks */
    return mcpwm_comparator_set_compare_value(comparator, ticks);
}
