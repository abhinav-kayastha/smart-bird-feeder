//
// Created by Keijo Länsikunnas on 18.2.2024.
//

#ifndef PICO_MODBUS_FRAMEBUF_H
#define PICO_MODBUS_FRAMEBUF_H

#include <string>
#include <cstdint>

class framebuf {
public:
    framebuf(uint16_t width_, uint16_t heigth_);
    virtual ~framebuf() = default;
    void fill(uint32_t color);
    void line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint32_t color);
    void hline(uint16_t x, uint16_t y, uint16_t w, uint32_t color);
    void vline(uint16_t x, uint16_t y, uint16_t h, uint32_t color);
    void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color, bool fill = false);
    void text(const char *str, uint16_t x, uint16_t y, uint32_t color = 1);
    void text(const std::string &str, uint16_t x, uint16_t y, uint32_t color = 1);
    void blit(framebuf &fb, int16_t x, int16_t y, uint32_t key = 0xFFFF, const framebuf *palette = nullptr);
    void scroll(int16_t xstep, int16_t ystep);
private:
    virtual void setpixel(uint16_t x, uint16_t y, uint32_t color) = 0;
    virtual uint32_t getpixel(uint16_t x, uint16_t y) const = 0;
    virtual void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) = 0;
protected:
    uint16_t width;
    uint16_t height;
};


#endif //PICO_MODBUS_FRAMEBUF_H
