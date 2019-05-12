#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "rbprotocol.h"
#include "rbwebserver.h"

#include "RBControl_wifi.hpp"

#include <iostream>
#include <memory>

#include <sys/time.h>

#include "pin.hpp"
#include "pwm.hpp"

const char OWNER[] = "vstill";
const char NAME[] = "lights";

#if __has_include("wifi_config.hpp")
#include "wifi_config.hpp"
#else
#error wifi not set, please provide WIFI_NAME & WIFI_PASSWORD in "wifi_config.hpp"
#endif

extern "C" void app_main() {
    rb::WiFi::connect(WIFI_NAME, WIFI_PASSWORD);
    rb_web_start(80);   // Start web server with control page (see data/index.html)

    ledc_fade_func_install( 0 );
    bool led_on = false;
    OutPin< 33_pin > led_pin;
    PWM< 25_pin, 0_timer, 0_channel, 16_tbits, inverted_logic > brightness_pwm( 100_Hz );
    brightness_pwm.set_duty_perc( 0 );

    // Initialize the communication protocol
    rb::Protocol prot(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, [&](const std::string& command, rbjson::Object *pkt) {
        if(command == "brightness") {
            int value = pkt->getInt( "brightness" );
            std::cout << "Changing brightness to " << value << std::endl;
            prot.send_log( "ack brightness %d\n", value );
            brightness_pwm.set_duty_perc( value );
        }
        else if (command == "ping") {
            int rcv = pkt->getInt( "id" );
            int64_t time = pkt->getInt( "time" );
            std::cout << "ping " << rcv << ", time is " << time << std::endl;
            struct timeval now = { .tv_sec = time_t( time / 1000 ), .tv_usec = time_t(time % 1000) * 1000 };
            settimeofday( &now, nullptr );

            rbjson::Object data;
            data.set( "id", new rbjson::Number( rcv ) );
            data.set( "time", new rbjson::Number( time ) );
            prot.send( "pong", &data );

            led_pin.set( led_on = !led_on );
        }
    });

    prot.start();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("%s's '%s' started!\n", OWNER, NAME);

    while( true ) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if ( prot.is_possessed() ) {
            rbjson::Object data;
            data.set( "status", "test msg" );
            prot.send( "status", &data );
        }
    }
}
