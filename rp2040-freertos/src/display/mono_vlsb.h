//
// Created by Keijo Länsikunnas on 18.2.2024.
//

#ifndef PICO_MODBUS_MONO_VLSB_H
#define PICO_MODBUS_MONO_VLSB_H
#include <memory>
#include "framebuf.h"

class mono_vlsb : public framebuf {
public:
    mono_vlsb(uint16_t width_, uint16_t height_, uint16_t stride_ = 0, uint16_t buf_offset = 0);
    mono_vlsb(const uint8_t *image, uint8_t width_, uint16_t height_, uint16_t stride_ = 0, uint16_t buf_offset = 0);
private:
    void setpixel(uint16_t x, uint16_t y, uint32_t color) override;
    uint32_t getpixel(uint16_t x, uint16_t y) const override;
    void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) override;
protected:
    uint32_t size;
    uint16_t stride;
    uint16_t buffer_offset;
    std::shared_ptr<uint8_t> buffer;
};


#endif //PICO_MODBUS_MONO_VLSB_H
