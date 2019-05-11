#ifndef PIN_HPP
#define PIN_HPP

#include <experimental/optional>

#include "driver/gpio.h"

#include "time.hpp"

namespace std
{
    using experimental::optional;
    using experimental::nullopt;
}

class Pin
{
    const gpio_num_t pin;

  public:
    Pin(gpio_num_t pin) : pin(pin) {}
    inline explicit Pin(int pin);

    operator gpio_num_t() const { return pin; }

    inline void setMode(gpio_mode_t mode,
                        gpio_pullup_t pull_up_en,
                        gpio_pulldown_t pull_down_en,
                        gpio_int_type_t intr_type);
    inline void setMode(gpio_mode_t mode);

    inline bool getLevel() const;
    inline void setLevel(bool level);
    inline void set_level_for_time(bool level, time_us time);

    inline std::optional<time_us> get_pulse_duration_us(bool level, time_us max_duration);

};

#include <cassert>
#include <iostream>
#include <stdexcept>

inline Pin::Pin(int pin) : pin(static_cast<gpio_num_t>(pin))
{
    assert(pin < GPIO_NUM_MAX);
}

inline void Pin::setMode(gpio_mode_t mode,
                         gpio_pullup_t pull_up_en,
                         gpio_pulldown_t pull_down_en,
                         gpio_int_type_t intr_type)
{
    gpio_config_t gpioConf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = mode,
        .pull_up_en = pull_up_en,
        .pull_down_en = pull_down_en,
        .intr_type = intr_type};

    gpio_config(&gpioConf);
}
inline void Pin::setMode(gpio_mode_t mode)
{
    switch (mode)
    {
    case GPIO_MODE_OUTPUT:
        setMode(GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE);
        setLevel(false);
        break;
    case GPIO_MODE_INPUT:
        setMode(GPIO_MODE_INPUT, GPIO_PULLUP_ENABLE, GPIO_PULLDOWN_ENABLE, GPIO_INTR_DISABLE);
        break;
    default:
        std::cerr << "Do not use this function for other modes than INPUT or OUTPUT (use overloaded version)" << std::endl;
        throw std::runtime_error("Do not use this function for other modes than INPUT or OUTPUT (use overloaded version)");
    }
}

inline bool Pin::getLevel() const
{
    return gpio_get_level(pin) > 0;
}

inline void Pin::setLevel(bool level)
{
    gpio_set_level(pin, level ? 1 : 0);
}

void Pin::set_level_for_time(bool level, time_us time)
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    setLevel(level);
    ets_delay_us(time.value);
    setLevel(!level);

    portEXIT_CRITICAL(&mux);
}

std::optional<time_us> Pin::get_pulse_duration_us(bool level, time_us max_duration)
{
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    // Previous ping isn't ended
    if (getLevel() == level)
    {
        portEXIT_CRITICAL(&mux);
        std::cout << "Pulse already started" << std::endl;
        return std::nullopt;
    }

    // Wait for echo
    time_us wait_timeout = time_us(get_time_us().value + max_duration.value);
    while (getLevel() != level)
    {
        if (get_time_us().value > wait_timeout.value)
        {
            portEXIT_CRITICAL(&mux);
            std::cout << "No pulse timeout (" << max_duration.value << " us)" << std::endl;
            return std::nullopt;
        }
    }

    // got echo, measuring
    time_us echo_start = get_time_us();
    time_us curr_time = echo_start;
    time_us meas_timeout = time_us(echo_start.value + max_duration.value);
    while (getLevel())
    {
        curr_time = get_time_us();
        if (curr_time.value > meas_timeout.value)
        {
            portEXIT_CRITICAL(&mux);
            std::cout << "Pulse too long (" << max_duration.value << " us)" << std::endl;
            return std::nullopt;
        }
    }

    portEXIT_CRITICAL(&mux);
    return time_us(curr_time.value - echo_start.value);
}

#endif // PIN_HPP
