/**
 * Copyright (c) 2016 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup bootloader_secure_ble main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file for secure DFU.
 *
 */

#include <stdint.h>
#include "boards.h"
#include "nrf_mbr.h"
#include "nrf_bootloader.h"
#include "nrf_bootloader_app_start.h"
#include "nrf_bootloader_dfu_timers.h"
#include "nrf_dfu.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_bootloader_info.h"
#include "nrf_delay.h"
#include "led_softblink.h"
#include "app_timer.h"
#include "nrf_clock.h"

#define LAIRD_LED_SIO    13
#define LAIRD_LED        0b10000000000000       //Bit-mask format

/* Timer used to blink LED on DFU progress. */
APP_TIMER_DEF(m_dfu_progress_led_timer);

static void on_error(void)
{
    NRF_LOG_FINAL_FLUSH();

#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_RTT)
    // To allow the buffer to be flushed by the host.
    nrf_delay_ms(100);
#endif
#ifdef NRF_DFU_DEBUG_VERSION
    NRF_BREAKPOINT_COND;
#endif
    NVIC_SystemReset();
}


void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    NRF_LOG_ERROR("%s:%d", p_file_name, line_num);
    on_error();
}


void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
    NRF_LOG_ERROR("Received a fault! id: 0x%08x, pc: 0x%08x, info: 0x%08x", id, pc, info);
    on_error();
}


void app_error_handler_bare(uint32_t error_code)
{
    NRF_LOG_ERROR("Received an error: 0x%08x!", error_code);
    on_error();
}


static void dfu_progress_led_timeout_handler(void * p_context)
{
    app_timer_id_t timer = (app_timer_id_t)p_context;

    uint32_t err_code = app_timer_start(timer,
                                        APP_TIMER_TICKS(DFU_LED_CONFIG_PROGRESS_BLINK_MS),
                                        p_context);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_pin_toggle(LAIRD_LED_SIO);
}


/**
 * @brief Function notifies certain events in DFU process.
 */
static void dfu_observer(nrf_dfu_evt_type_t evt_type)
{
    static bool timer_created = false;
    uint32_t err_code;

    if (!timer_created)
    {
        err_code = app_timer_create(&m_dfu_progress_led_timer,
                                    APP_TIMER_MODE_SINGLE_SHOT,
                                    dfu_progress_led_timeout_handler);
        APP_ERROR_CHECK(err_code);
        timer_created = true;
    }

    switch (evt_type)
    {
        case NRF_DFU_EVT_DFU_FAILED:
        case NRF_DFU_EVT_DFU_ABORTED:
            err_code = led_softblink_stop();
            APP_ERROR_CHECK(err_code);

            err_code = app_timer_stop(m_dfu_progress_led_timer);
            APP_ERROR_CHECK(err_code);

            err_code = led_softblink_start(LAIRD_LED);
            APP_ERROR_CHECK(err_code);
            break;
        case NRF_DFU_EVT_DFU_INITIALIZED:
            nrf_gpio_cfg(LAIRD_LED_SIO, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);

            if (!nrf_clock_lf_is_running())
            {
                nrf_clock_task_trigger(NRF_CLOCK_TASK_LFCLKSTART);
            }
            err_code = app_timer_init();
            APP_ERROR_CHECK(err_code);

            led_sb_init_params_t led_sb_init_param = LED_SB_INIT_DEFAULT_PARAMS(LAIRD_LED);

            uint32_t ticks = APP_TIMER_TICKS(DFU_LED_CONFIG_TRANSPORT_INACTIVE_BREATH_MS);
            led_sb_init_param.p_leds_port    = BSP_LED_1_PORT;
            led_sb_init_param.on_time_ticks  = ticks;
            led_sb_init_param.off_time_ticks = ticks;
            led_sb_init_param.duty_cycle_max = 255;
            led_sb_init_param.active_high = true;

            err_code = led_softblink_init(&led_sb_init_param);
            APP_ERROR_CHECK(err_code);

            err_code = led_softblink_start(LAIRD_LED);
            APP_ERROR_CHECK(err_code);
            break;
        case NRF_DFU_EVT_TRANSPORT_ACTIVATED:
        {
            uint32_t ticks = APP_TIMER_TICKS(DFU_LED_CONFIG_TRANSPORT_ACTIVE_BREATH_MS);
            led_softblink_off_time_set(ticks);
            led_softblink_on_time_set(ticks);
        }
            break;
        case NRF_DFU_EVT_TRANSPORT_DEACTIVATED:
        {
            uint32_t ticks =  APP_TIMER_TICKS(DFU_LED_CONFIG_PROGRESS_BLINK_MS);
            err_code = led_softblink_stop();
            APP_ERROR_CHECK(err_code);

            err_code = app_timer_start(m_dfu_progress_led_timer, ticks, m_dfu_progress_led_timer);
            APP_ERROR_CHECK(err_code);
            break;
        }
        case NRF_DFU_EVT_DFU_STARTED:
            break;
        default:
            break;
    }
}


/**@brief Function for application main entry. */
int main(void)
{
    uint32_t ret_val;

    // Must happen before flash protection is applied, since it edits a protected page.
    nrf_bootloader_mbr_addrs_populate();

    // Protect MBR and bootloader code from being overwritten.
    ret_val = nrf_bootloader_flash_protect(0, MBR_SIZE, false);
    APP_ERROR_CHECK(ret_val);
    ret_val = nrf_bootloader_flash_protect(BOOTLOADER_START_ADDR, BOOTLOADER_SIZE, false);
    APP_ERROR_CHECK(ret_val);

    (void) NRF_LOG_INIT(nrf_bootloader_dfu_timer_counter_get);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("Inside main");

    ret_val = nrf_bootloader_init(dfu_observer);
    APP_ERROR_CHECK(ret_val);

    NRF_LOG_FLUSH();

    NRF_LOG_ERROR("After main, should never be reached.");
    NRF_LOG_FLUSH();

    APP_ERROR_CHECK_BOOL(false);
}

/**
 * @}
 */
