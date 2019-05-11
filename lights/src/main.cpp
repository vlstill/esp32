#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>

#include "rbprotocol.h"
#include "rbwebserver.h"

#include "RBControl_wifi.hpp"

#include "pin.hpp"

#include <iostream>
#include <memory>

#define OWNER "vstill"
#define NAME "test"

#if __has_include("wifi_config.hpp")
#include "wifi_config.hpp"
#endif

#ifndef WIFI_NAME
#define WIFI_NAME "xxx"
#define WIFI_PASSWORD "xxx"
#endif

#ifndef AP_WIFI_NAME
#define AP_WIFI_NAME "Module-wifi"
#define AP_WIFI_PASSWORD "paradise"
#endif

extern "C" void app_main() {
    rb::WiFi::connect(WIFI_NAME, WIFI_PASSWORD);
//    rb::WiFi::startAp(AP_WIFI_NAME, AP_WIFI_PASSWORD);
    rb_web_start(80);   // Start web server with control page (see data/index.html)

    // Initialize the communication protocol
    rb::Protocol prot(OWNER, NAME, "Compiled at " __DATE__ " " __TIME__, [&](const std::string& command, rbjson::Object *pkt) {
        if(command == "brightness") {
            int value = pkt->getObject( "data" )->getInt( "brightness" );
            std::cout << "Changing brightness to " << value << std::endl;
            prot.send_log( "ack brightness %d\n", value );
        }
        else if (command == "ping") {
            int rcv = pkt->getInt( "id" );
            std::cout << "ping " << rcv << std::endl;
            std::unique_ptr< rbjson::Object > data{ new rbjson::Object() };
            data->set( "id", new rbjson::Number( rcv ) );
            prot.send( "pong", data.get() );
        }
    });

    prot.start();

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("%s's '%s' started!\n", OWNER, NAME);

    while( true ) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if ( prot.is_possessed() ) {
        }
    }
}
