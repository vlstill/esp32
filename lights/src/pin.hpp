#pragma once

#include "driver/gpio.h"


constexpr inline gpio_num_t operator""_pin( unsigned long long pin ) {
    assert( pin < GPIO_NUM_MAX );
    return gpio_num_t( pin );
}

template< gpio_num_t n >
struct OutPin
{
    static constexpr gpio_num_t PIN_NUM = n;

    OutPin() {
        gpio_pad_select_gpio( PIN_NUM );
        gpio_set_direction( PIN_NUM, GPIO_MODE_OUTPUT );
    }

    void set( bool v = true ) {
        gpio_set_level( PIN_NUM, v );
    }
};
