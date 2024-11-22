//
// Created by Keijo Länsikunnas on 10.9.2024.
//
#include <mutex>
#include "pico/stdlib.h"
#include "PicoI2C.h"

//#define DEBUG_PRINT

#ifdef DEBUG_PRINT
#include "Syslog.h"
#endif

#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17

#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

PicoI2C *PicoI2C::i2c0_instance{nullptr};
PicoI2C *PicoI2C::i2c1_instance{nullptr};

void PicoI2C::i2c0_irq() {
    if (i2c0_instance) i2c0_instance->isr();
    else irq_set_enabled(I2C0_IRQ, false);
}

void PicoI2C::i2c1_irq() {
    if (i2c1_instance) i2c1_instance->isr();
    else irq_set_enabled(I2C1_IRQ, false); // disable interrupt if we don't have instance
}

PicoI2C::PicoI2C(uint bus_nr, uint speed) :
        task_to_notify(nullptr), wbuf{nullptr}, wctr{0}, rbuf{nullptr}, rctr{0}, rcnt{0} {
    int scl = I2C0_SCL_PIN;
    int sda = I2C0_SDA_PIN;
    switch (bus_nr) {
        case 0:
            i2c = i2c0;
            irqn = I2C0_IRQ;
            break;
        case 1:
            i2c = i2c1;
            irqn = I2C1_IRQ;
            scl = I2C1_SCL_PIN;
            sda = I2C1_SDA_PIN;
            break;
        default:
            panic("Invalid I2C bus number\n");
            break;
    }
    gpio_init(scl);
    gpio_pull_up(scl);
    gpio_init(sda);
    gpio_pull_up(sda);
    irq_set_enabled(irqn, false);
    irq_set_exclusive_handler(irqn, bus_nr ? i2c1_irq : i2c0_irq);
    i2c_init(i2c, speed);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    // Set FIFO watermarks
    i2c->hw->tx_tl = 0; // TX_FIFO watermark to 0
    i2c->hw->rx_tl = 14; // RX_FIFO watermark to 15 (manual gives impression that level is one higher than reg value)
    if (bus_nr) i2c1_instance = this;
    else i2c0_instance = this;
}


void PicoI2C::tx_fill_fifo() {
#ifdef DEBUG_PRINT
    int fill{0};
#endif
    while (wctr > 0 && i2c_get_write_available(i2c) > 0) {
        bool last = wctr == 1;
        bool stop = rctr == 0;
        i2c->hw->data_cmd =
                // There may be a restart needed instead of (stop)-start
                bool_to_bit(i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                // stop is needed if this is last write and there is no read after this
                bool_to_bit(last && stop) << I2C_IC_DATA_CMD_STOP_LSB |
                *wbuf++;
        // clear restart after first write
        if (i2c->restart_on_next) i2c->restart_on_next = false;
        --wctr;

        if (last && !stop) i2c->restart_on_next = true;
#ifdef DEBUG_PRINT
        ++fill;
#endif
    }
#ifdef DEBUG_PRINT
    Syslog::debug("tx_fill: %d", fill);
#endif
}


void PicoI2C::rx_fill_fifo() {
#ifdef DEBUG_PRINT
    int fill{0};
#endif

    while (rctr > 0 && i2c_get_write_available(i2c) > 0) {
        bool last = rctr == 1;
        i2c->hw->data_cmd =
                // There may be a restart needed instead of (stop)-start
                bool_to_bit(i2c->restart_on_next) << I2C_IC_DATA_CMD_RESTART_LSB |
                // Read is always at the last transaction so stop is issued after last command
                bool_to_bit(last) << I2C_IC_DATA_CMD_STOP_LSB |
                I2C_IC_DATA_CMD_CMD_BITS; // -> 1 for read;
        // clear restart bit after first command
        if (i2c->restart_on_next) i2c->restart_on_next = false;
        --rctr;
#ifdef DEBUG_PRINT
        ++fill;
#endif
    }

#ifdef DEBUG_PRINT
    Syslog::debug("rx_fill: %d", fill);
#endif
}


uint PicoI2C::write(uint8_t addr, const uint8_t *buffer, uint length) {
    return transaction(addr, buffer, length, nullptr, 0);
}


uint PicoI2C::read(uint8_t addr, uint8_t *buffer, uint length) {
    return transaction(addr, nullptr, 0, buffer, length);
}


uint PicoI2C::transaction(uint8_t addr, const uint8_t *wbuffer, uint wlength, uint8_t *rbuffer, uint rlength) {
    assert((wbuffer && wlength > 0) || (rbuffer && rlength > 0));
    std::lock_guard<Fmutex> exclusive(access);
    task_to_notify = xTaskGetCurrentTaskHandle();

    i2c->hw->enable = 0;
    i2c->hw->tar = addr;
    i2c->hw->enable = 1;
    i2c->hw->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_TX_EMPTY_BITS | I2C_IC_INTR_MASK_M_RX_FULL_BITS;
    i2c->restart_on_next = false;
    // setup transfer
    wbuf = wbuffer;
    wctr = wlength;
    rbuf = rbuffer;
    rctr = rlength; // for writing read commands
    rcnt = rlength; // for counting received bytes

    // write is done first if we have a combined transaction
    if (wctr > 0) tx_fill_fifo();
    else rx_fill_fifo();

    uint count = wlength + rlength;
    // enable interrupts
    irq_set_enabled(irqn, true);
    // wait for stop interrupt
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) == 0) {
        // timed out
        count = 0;
    } else {
        count -= rcnt + wctr;
    }
    irq_set_enabled(irqn, false);

    // if count !=  sum of lengths transaction failed
    return count;
}


void PicoI2C::isr() {
    BaseType_t hpw = pdFALSE;
#ifdef DEBUG_PRINT
    Syslog::debug("%d %d %d %d",
        !!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS),
        !!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_RX_FULL_BITS),
        !!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS),
        !!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_RX_OVER_BITS)
        );
#endif
    // See if we have active read and data available.
    // Read is paced writes to command register in master mode
    // so we don't need RX_FULL interrupt.
    // We just empty the rxfifo before additional writes to cmd register
    //  Testing (i2c->hw->status & I2C_IC_STATUS_RFNE_BITS) works also
#ifdef DEBUG_PRINT
    int fill{0};
#endif
    while (rcnt > 0 && i2c->hw->rxflr > 0) {
        *rbuf++ = static_cast<uint8_t>(i2c->hw->data_cmd);
        --rcnt;
#ifdef DEBUG_PRINT
        ++fill;
#endif
    }
#ifdef DEBUG_PRINT
    Syslog::debug("read: %d, %d", fill, rcnt);
#endif

    if (i2c->hw->intr_stat & I2C_IC_INTR_MASK_M_TX_EMPTY_BITS) {
        // write commands go first
        if (wctr > 0) {
            tx_fill_fifo();
        } else if (rctr > 0) {
            rx_fill_fifo();
        }
        if (wctr == 0 && rctr == 0) {
            // Disable TX_EMPTY interrupt when we are done with write and read commands
            // keep receive and stop interrupts active to catch received bytes and stop
            i2c->hw->intr_mask = I2C_IC_INTR_MASK_M_STOP_DET_BITS | I2C_IC_INTR_MASK_M_RX_FULL_BITS;
        }
    }

    // notify if we are done - hw should also issue a stop if transaction is aborted
    if (i2c->hw->intr_stat & I2C_IC_INTR_MASK_M_STOP_DET_BITS) {
        i2c->hw->intr_mask = 0; // mask all interrupts
        (void) i2c->hw->clr_stop_det;
        xTaskNotifyFromISR(task_to_notify, 1, eSetValueWithOverwrite, &hpw);
    }
    portYIELD_FROM_ISR(hpw);
}

