//
// Created by Keijo Länsikunnas on 30.8.2024.
//

#include "PicoOsUart.h"
#include <mutex>
#include <hardware/gpio.h>
#include <cstring>



static PicoOsUart *pu0;
static PicoOsUart *pu1;


void pico_uart0_handler() {
    if(pu0) {
        pu0->uart_irq_rx();
        pu0->uart_irq_tx();
    }
    else irq_set_enabled(UART0_IRQ, false);
}

void pico_uart1_handler() {
    if(pu1) {
        pu1->uart_irq_rx();
        pu1->uart_irq_tx();
    }
    else irq_set_enabled(UART1_IRQ, false);
}


PicoOsUart::PicoOsUart(int uart_nr, int tx_pin, int rx_pin, int speed, int stop, int tx_size, int rx_size) : speed{speed} {
    tx = xQueueCreate(tx_size, sizeof(char));
    rx = xQueueCreate(rx_size, sizeof(char));
    irqn = uart_nr==0 ? UART0_IRQ : UART1_IRQ;
    uart = uart_nr==0 ? uart0 : uart1;
    if(uart_nr == 0) {
        pu0 = this;
    }
    else {
        pu1 = this;
    }

    // ensure that we don't get any interrupts from the uart during configuration
    irq_set_enabled(irqn, false);

    // Set up our UART with the required speed.
    uart_init(uart, speed);
    uart_set_format(uart, 8, stop, UART_PARITY_NONE);

    // Set the TX and RX pins by using the function select on the GPIO
    // See datasheet for more information on function select
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);

    irq_set_exclusive_handler(irqn, uart_nr == 0 ? pico_uart0_handler : pico_uart1_handler);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(uart, true, false);
    // enable UART0 interrupts on NVIC
    irq_set_enabled(irqn, true);
}

int PicoOsUart::read(uint8_t *buffer, int size, TickType_t timeout) {
    std::lock_guard<Fmutex> exclusive(access);
    int count = 0;
    while(count < size && xQueueReceive(rx, buffer, timeout) == pdTRUE) {
        ++buffer;
        ++count;
    }
    return count;
}

int PicoOsUart::write(const uint8_t *buffer, int size, TickType_t timeout) {
    std::lock_guard<Fmutex> exclusive(access);
    int count = 0;
    // write data to queue
    while(count < size && xQueueSendToBack(tx, buffer, timeout) == pdTRUE) {
        ++buffer;
        ++count;
    }

    // disable interrupts on NVIC while managing transmit interrupts
    irq_set_enabled(irqn, false);
    // if transmit interrupt is not enabled we need to enable it and give fifo an initial filling
    if(!(uart_get_hw(uart)->imsc & (1 << UART_UARTIMSC_TXIM_LSB))) {
        uint8_t ch;
        // fifo requires initial fill to get TX interrupts going
        while(uart_is_writable(uart) && xQueueReceive(tx, &ch, 0) == pdTRUE) {
            uart_get_hw(uart)->dr = ch;
        }
        // enable interrupt only if there is data left in the queue
        if(uxQueueMessagesWaiting(tx)>0) {
            // enable transmit interrupt
            uart_set_irq_enables(uart, true, true);
        }
    }
    // enable interrupts on NVIC
    irq_set_enabled(irqn, true);

    return count;
}

int PicoOsUart::send(const char *str) {
    write(reinterpret_cast<const uint8_t *>(str), static_cast<int>(strlen(str)));
    return 0;
}

int PicoOsUart::send(const std::string &str) {
    write(reinterpret_cast<const uint8_t *>(str.c_str()), static_cast<int>(str.length()));
    return 0;
}

int PicoOsUart::flush() {
    std::lock_guard<Fmutex> exclusive(access);
    int count = 0;
    char dummy = 0;
    while(xQueueReceive(rx, &dummy, 0)==pdTRUE) {
        ++count;
    }
    return count;
}

void PicoOsUart::uart_irq_rx() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    while(uart_is_readable(uart)) {
        uint8_t c = uart_getc(uart);
        // ignoring return value for now
        xQueueSendToBackFromISR(rx, &c, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void PicoOsUart::uart_irq_tx() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t ch;
    while(uart_is_writable(uart) && xQueueReceiveFromISR(tx, &ch, &xHigherPriorityTaskWoken) == pdTRUE) {
        uart_get_hw(uart)->dr = ch;
    }

    if (xQueueIsQueueEmptyFromISR(tx)) {
        // disable tx interrupt if transmit buffer is empty
        uart_set_irq_enables(uart, true, false);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int PicoOsUart::get_fifo_level() {
    const uint8_t flv[]={4, 8,16, 24, 28, 0, 0, 0, 0 };
    // figure out fifo level to calculate timeout
    uint32_t lcr_h = uart_get_hw(uart)->lcr_h;
    uint32_t fcr = (uart_get_hw(uart)->ifls >> 3) & 0x7;
    // if fifo is enabled we need to take into account delay caused by the fifo
    if(!(lcr_h | UART_UARTLCR_H_FEN_BITS)) {
        fcr = 8; // last is dummy entry that is outside of normal fcr range. it is used to ensure we return zero
    }
    return flv[fcr];
}

int PicoOsUart::get_baud() const {
    return speed;
}



