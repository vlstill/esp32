#ifndef TIME_HPP
#define TIME_HPP

#include <cassert>
#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct time_ms
{
    unsigned long long value;
    explicit time_ms(unsigned long long time) : value(time) {}
};

inline time_ms operator"" _ms(unsigned long long time)
{
    return time_ms(time);
}

inline time_ms operator"" _s(unsigned long long time)
{
    assert(time <= 1000 * time);
    return time_ms(1000 * time);
}

struct time_us
{
    unsigned long long value;
    explicit time_us(unsigned long long time) : value(time) {}
};

inline time_us operator"" _us(unsigned long long time)
{
    return time_us(time);
}

inline void delay(time_ms time)
{
    assert(pdMS_TO_TICKS(time.value) <= portMAX_DELAY);
    if (time.value != 0 && pdMS_TO_TICKS(time.value) == 0)
    {
        std::cout << "Delay " << time.value << " ms is too short, skipping" << std::endl;
    }
    vTaskDelay(pdMS_TO_TICKS(time.value));
}

inline time_us get_time_us()
{
    return time_us(esp_timer_get_time());
}

#endif // TIME_HPP
