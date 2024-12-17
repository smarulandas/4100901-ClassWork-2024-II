#include <stdint.h>

#include "systick.h"
#include "gpio.h"
#include "uart.h"


int main(void)
{
    configure_systick_and_start();
    configure_gpio();
    
    UART_Init(USART2);

    uint8_t state = 0; // state of the FSM
    UART_send_string(USART2, "Hello World, from main!\r\n");

    uint8_t buffer[10];
    UART_receive_string(USART2, buffer, 10);

    UART_send_string(USART2, "Received: ");
    UART_send_string(USART2, (char *)buffer);
    UART_send_string(USART2, "\r\n");

    UART_receive_it(USART2, buffer, 10);
    while (1) {
        if (rx_ready != 0) {
            UART_send_string(USART2, "Received: ");
            UART_send_string(USART2, (char *)buffer);
            UART_send_string(USART2, "\r\n");
            UART_receive_it(USART2, buffer, 10);
            rx_ready = 0;
        }
        switch (state) {
        case 0: // idle
            if (gpio_button_is_pressed() != 0) { // If button is pressed
                state = 1;
            } else if (systick_GetTick() >= 500) { // Blink LED every 500 ms
                state = 2;
            }
            break;
        case 1: // button pressed
            if (gpio_button_is_pressed() == 0) { // If button is released
                systick_reset(); // Reset counter
                state = 0;
            }
            break;
        case 2: // led toggle
            gpio_toggle_led();
            systick_reset(); // Reset counter
            state = 0;
            break;
        default:
            break;
        }
        
    }
}

