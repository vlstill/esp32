#pragma once

#include <cstdio>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"


#define TIMER_DIVIDER         16  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (3.4179) // sample test interval for the first timer
#define TEST_WITH_RELOAD 1 // testing will be done with auto reload

struct TimerEvent
{
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
};

struct Timer
{
    Timer( int timer_idx, bool auto_reload, double timer_interval_sec,
           size_t divider = 16 )
    {
        timer_config_t config;
        config.divider = divider;
        config.counter_dir = TIMER_COUNT_UP;
        config.counter_en = TIMER_PAUSE;
        config.alarm_en = TIMER_ALARM_EN;
        config.intr_type = TIMER_INTR_LEVEL;
        config.auto_reload = auto_reload;
        timer_init( TIMER_GROUP_0, timer_idx, &config );

        /* Timer's counter will initially start from value below.
        Also, if auto_reload is set, this value will be automatically reload on alarm */
        timer_set_counter_value( TIMER_GROUP_0, timer_idx, 0ULL );

        /* Configure the alarm value and the interrupt on alarm. */
        timer_set_alarm_value( TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE );
        timer_enable_intr( TIMER_GROUP_0, timer_idx );
        timer_isr_register( TIMER_GROUP_0, timer_idx, timer_group0_isr,
                            static_cast< void * >( timer_idx ), ESP_INTR_FLAG_IRAM, nullptr );

        timer_start( TIMER_GROUP_0, timer_idx );
    }
};
