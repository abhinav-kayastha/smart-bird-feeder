//
// Created by Keijo Länsikunnas on 30.8.2024.
//

#ifndef RP2040_FREERTOS_IRQ_PICOOSUART_H
#define RP2040_FREERTOS_IRQ_PICOOSUART_H

#include <hardware/uart.h>
#include <hardware/irq.h>
#include <string>
#include "FreeRTOS.h"
#include "queue.h"
#include "Fmutex.h"

class PicoOsUart {
    friend void pico_uart0_handler(void);
    friend void pico_uart1_handler(void);
public:
    PicoOsUart(int uart_nr, int tx_pin, int rx_pin, int speed, int stop = 1, int tx_size = 256, int rx_size = 256);
    PicoOsUart(const PicoOsUart &) = delete; // prevent copying because each instance is associated with a HW peripheral
    int read(uint8_t *buffer, int size, TickType_t timeout = pdMS_TO_TICKS(500));
    int write(const uint8_t *buffer, int size, TickType_t timeout = pdMS_TO_TICKS(500));
    int send(const char *str);
    int send(const std::string &str);
    int flush();
    int get_fifo_level();
    int get_baud() const;
private:
    void uart_irq_rx();
    void uart_irq_tx();
    Fmutex access;
    QueueHandle_t tx;
    QueueHandle_t rx;
    uart_inst_t *uart;
    int irqn;
    int speed;
};


#endif //RP2040_FREERTOS_IRQ_PICOOSUART_H
