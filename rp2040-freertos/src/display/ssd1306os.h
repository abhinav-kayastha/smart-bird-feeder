//
// Created by Keijo Länsikunnas on 16.9.2024.
//

#ifndef RP2040_FREERTOS_IRQ_SSD1306OS_H
#define RP2040_FREERTOS_IRQ_SSD1306OS_H

#include <memory>

#include "mono_vlsb.h"
#include "PicoI2C.h"

class ssd1306os : public mono_vlsb {
public:
    explicit ssd1306os(std::shared_ptr<PicoI2C> i2c, uint16_t device_address = 0x3C, uint16_t width = 128, uint16_t height = 64);
    void show();
private:
    void init();
    void send_cmd(uint8_t value);
    std::shared_ptr<PicoI2C> ssd1306_i2c;
    uint8_t address;
};



#endif //RP2040_FREERTOS_IRQ_SSD1306OS_H
