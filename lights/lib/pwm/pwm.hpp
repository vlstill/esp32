#ifndef PWM_HPP
#define PWM_HPP

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
#include "time.hpp"

/*
static pwm si musí pamatovat:
když zadám gpio_num:
    speed_mode,
    timer_num,
    duty_resolution,
    channel,
    intr_type

*/

class Pwm
{
    struct pwm_config_t
    {
        ledc_mode_t speed_mode;
        ledc_timer_t timer_num;
        ledc_timer_bit_t duty_resolution;
        ledc_channel_t channel;
        ledc_intr_type_t intr_type;
    };

    inline static bool initialized = false;
    inline static SemaphoreHandle_t binarySemphr = NULL;
    inline static std::map<gpio_num_t, pwm_config_t> gpioInUse = {};
    inline static std::set<std::pair<ledc_mode_t, ledc_channel_t>> channelsInUse = {};
    inline static std::multiset<std::pair<ledc_mode_t, ledc_timer_t>> timersInUse = {};

    gpio_num_t gpio_num;

    ledc_channel_t getFreeChannel(ledc_mode_t speed_mode)
    {
        for (int i = 0; i < static_cast<int>(LEDC_CHANNEL_MAX); i++)
        {
            ledc_channel_t channel = static_cast<ledc_channel_t>(i);
            if (channelsInUse.count(std::pair(speed_mode, channel)) == 0)
            {
                return channel;
            }
        }
        throw std::runtime_error("no available pwm channel");
    }

    ledc_timer_t getFreeTimer(const freqHz &freq, ledc_mode_t speed_mode)
    {
        for (const auto &iter : timersInUse)
        {
            if (speed_mode == iter.first)
            {
                if (freq.value == ledc_get_freq(iter.first, iter.second))
                {
                    return iter.second;
                }
            }
        }

        for (int i = 0; i < static_cast<int>(LEDC_TIMER_MAX); i++)
        {
            ledc_timer_t timer = static_cast<ledc_timer_t>(i);
            if (timersInUse.count(std::pair(speed_mode, timer)) == 0)
            {
                return timer;
            }
        }
        return LEDC_TIMER_MAX; // No available timer
    }

    void configPwm(const freqHz &freq,
                   ledc_mode_t speed_mode,
                   ledc_timer_t timer_num,
                   ledc_timer_bit_t duty_resolution,
                   ledc_channel_t channel,
                   ledc_intr_type_t intr_type)
    {
        std::cout << "Initializing PWM on gpio pin " << gpio_num;
        std::cout << " in speed mode ";
        if (speed_mode == LEDC_HIGH_SPEED_MODE)
            std::cout << "high_speed";
        else
            std::cout << "low_speed";
        std::cout << " with timer " << timer_num;
        std::cout << ", channel " << channel;
        std::cout << ", with frequency " << freq.value;
        std::cout << ", with duty resolution " << duty_resolution << " bits";
        std::cout << ", with interrupts ";
        if (intr_type == LEDC_INTR_DISABLE)
            std::cout << "disabled";
        else
            std::cout << "enabled";
        std::cout << std::endl;

        assert(xSemaphoreTake(binarySemphr, portMAX_DELAY) == pdTRUE);

        auto empl_pair = gpioInUse.try_emplace(gpio_num,
                                               pwm_config_t{
                                                   .speed_mode = speed_mode,
                                                   .timer_num = timer_num,
                                                   .duty_resolution = duty_resolution,
                                                   .channel = channel,
                                                   .intr_type = intr_type});
        pwm_config_t &conf = empl_pair.first->second;

        if (empl_pair.second)
        {
            channelsInUse.emplace(conf.speed_mode, conf.channel);
            timersInUse.emplace(conf.speed_mode, conf.timer_num);
        }
        else
        {
            channelsInUse.erase(std::pair(conf.speed_mode, conf.channel));
            timersInUse.erase(std::pair(conf.speed_mode, conf.timer_num));
            conf.speed_mode = speed_mode;
            conf.timer_num = timer_num;
            conf.duty_resolution = duty_resolution;
            conf.channel = channel;
            conf.intr_type = intr_type;
            channelsInUse.emplace(conf.speed_mode, conf.channel);
            timersInUse.emplace(conf.speed_mode, conf.timer_num);
        }

        assert(xSemaphoreGive(binarySemphr) == pdTRUE);


        ledc_timer_config_t timer_conf;
        timer_conf.speed_mode = conf.speed_mode;
        timer_conf.duty_resolution = conf.duty_resolution;
        timer_conf.timer_num = conf.timer_num;
        timer_conf.freq_hz = freq.value;
        ledc_timer_config(&timer_conf);

        ledc_channel_config_t ledc_conf;
        ledc_conf.gpio_num = gpio_num;
        ledc_conf.speed_mode = conf.speed_mode;
        ledc_conf.channel = conf.channel;
        ledc_conf.intr_type = conf.intr_type;
        ledc_conf.timer_sel = conf.timer_num;
        ledc_conf.duty = 0;
        ledc_conf.hpoint = 0;
        ledc_channel_config(&ledc_conf);
    }

  public:
    Pwm(Pin pin) : gpio_num(pin)
    {
        if (!initialized)
        {
            initialized = true;
            ledc_fade_func_install(0);
            binarySemphr = xSemaphoreCreateBinary();
            assert(xSemaphoreGive(binarySemphr) == pdTRUE);
        }
        assert(binarySemphr);
    }
    void startPwm(const freqHz &freq, ledc_timer_bit_t duty_resolution = LEDC_TIMER_15_BIT)
    {
        Pin(gpio_num).setMode(GPIO_MODE_OUTPUT);
        configPwm(freq,
                  LEDC_HIGH_SPEED_MODE,
                  getFreeTimer(freq, LEDC_HIGH_SPEED_MODE),
                  duty_resolution,
                  getFreeChannel(LEDC_HIGH_SPEED_MODE),
                  LEDC_INTR_DISABLE);
    }

    template <typename T>
    void setDutyPerc(T dutyPerc)
    {
        static_assert(std::is_arithmetic_v<T>);
        assert(0 <= dutyPerc && dutyPerc <= 100);

        auto iter = gpioInUse.find(gpio_num);
        assert(iter != gpioInUse.end());
        pwm_config_t &conf = iter->second;

        uint32_t duty = ((1UL << conf.duty_resolution) - 1) * dutyPerc / 100;

        ledc_set_duty_and_update(conf.speed_mode, conf.channel, duty, 0);
    }

    template <typename T>
    void setFadeWithTime(T targetDutyPerc, time_ms time, bool blocking = false)
    {
        static_assert(std::is_arithmetic_v<T>);
        assert(0 <= targetDutyPerc && targetDutyPerc <= 100);
        assert(time.value < (1ULL << 32));

        auto iter = gpioInUse.find(gpio_num);
        assert(iter != gpioInUse.end());
        pwm_config_t &conf = iter->second;

        uint32_t target_duty = ((1UL << conf.duty_resolution) - 1) * targetDutyPerc / 100;
        
        ledc_set_fade_time_and_start(conf.speed_mode,
                                     conf.channel,
                                     target_duty,
                                     time.value,
                                     blocking ? LEDC_FADE_WAIT_DONE : LEDC_FADE_NO_WAIT);
    }
};

#endif // PWM_HPP
