#pragma once

#include <cassert>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "freq.hpp"
#include "pin.hpp"

constexpr inline ledc_timer_t operator""_timer( unsigned long long pin ) {
    assert( pin < LEDC_TIMER_MAX );
    return ledc_timer_t( pin );
}

constexpr inline ledc_channel_t operator""_channel( unsigned long long pin ) {
    assert( pin < LEDC_CHANNEL_MAX );
    return ledc_channel_t( pin );
}

constexpr inline ledc_timer_bit_t operator""_tbits( unsigned long long pin ) {
    assert( pin < LEDC_TIMER_BIT_MAX );
    return ledc_timer_bit_t( pin );
}

static constexpr bool inverted_logic = true;

template< gpio_num_t pin,
          ledc_timer_t timer, ledc_channel_t channel, ledc_timer_bit_t bits,
          bool inverted = false >
struct PWM : OutPin< pin >
{
    static constexpr auto PIN_NUM = pin;
    static constexpr ledc_timer_t TIMER = timer;
    static constexpr ledc_channel_t CHANNEL = channel;
    static constexpr ledc_timer_bit_t BITS = bits;
    static constexpr uint32_t MAX_DUTY = (uint64_t( 1 ) << BITS);

    template <typename T>
    void set_duty_perc( T dutyPerc )
    {
        static_assert( std::is_arithmetic<T>::value,
                       "type used in set_duty_perc must be arithmetic" );
        assert( 0 <= dutyPerc );
        assert( dutyPerc <= 100 );
        uint32_t duty = MAX_DUTY * dutyPerc / 100;
        set_duty( duty );
    }

    void set_duty( uint32_t duty ) {
        ledc_set_duty_and_update( LEDC_HIGH_SPEED_MODE, CHANNEL,
                                  inverted ? MAX_DUTY - duty : duty, 0 );
    }

    PWM( FreqHz freq )
    {
        ledc_timer_config_t timer_conf;
        timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
        timer_conf.duty_resolution = BITS;
        timer_conf.timer_num = TIMER;
        timer_conf.freq_hz = freq.value;
        ledc_timer_config(&timer_conf);

        ledc_channel_config_t ledc_conf;
        ledc_conf.gpio_num = PIN_NUM;
        ledc_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
        ledc_conf.channel = CHANNEL;
        ledc_conf.intr_type = LEDC_INTR_DISABLE;
        ledc_conf.timer_sel = TIMER;
        ledc_conf.duty = 0;
        ledc_conf.hpoint = 0;
        ledc_channel_config(&ledc_conf);
    }
};
