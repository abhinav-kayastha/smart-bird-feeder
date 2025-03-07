//
// Created by Keijo Länsikunnas on 18.2.2024.
//

#ifndef PICO_MODBUS_SSD1306_H
#define PICO_MODBUS_SSD1306_H

#include "mono_vlsb.h"
#include "hardware/i2c.h"

class ssd1306 : public mono_vlsb {
public:
    explicit ssd1306(i2c_inst *i2c, uint16_t device_address = 0x3C, uint16_t width = 128, uint16_t height = 64);
    void show();
private:
    void init();
    void send_cmd(uint8_t value);
    i2c_inst *ssd1306_i2c;
    uint8_t address;
};


#endif //PICO_MODBUS_SSD1306_H
