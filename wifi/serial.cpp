#include "esp32.hpp"
#include <iostream>
#include <driver/uart.h>
#include <thread>
#include <chrono>
#include <esp_task_wdt.h>

using namespace std::literals;

void esp_main() {
    std::cout << "Hello!\n";

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 2048, 2048, 0, nullptr, 0));

    char test[] = "hello!\n";
    uart_write_bytes( UART_NUM_2, test, sizeof( test ) - 1 );
    std::cout << "written\n";

    std::string buff;
    while ( true ) {
        size_t length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len( UART_NUM_2, &length));
        buff.resize( length );
        length = uart_read_bytes( UART_NUM_2, reinterpret_cast< uint8_t * >( buff.data() ), length, 100 );
        buff.resize( length );
        if ( length > 0 )
            std::cout << buff << std::flush;
        esp_task_wdt_reset();
        vTaskDelay( 1 );
    }
}
