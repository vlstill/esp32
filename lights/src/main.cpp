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
#else
#error wifi not set, please provide WIFI_NAME & WIFI_PASSWORD in "wifi_config.hpp"
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
            rbjson::Object data;
            data.set( "id", new rbjson::Number( rcv ) );
            prot.send( "pong", &data );
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
