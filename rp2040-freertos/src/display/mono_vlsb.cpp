//
// Created by Keijo Länsikunnas on 18.2.2024.
//
// This is based on MicroPython modframebuf.c
// modframebuf.c licence attached below
/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <cstring>
#include "mono_vlsb.h"

mono_vlsb::mono_vlsb(uint16_t width_, uint16_t height_, uint16_t stride_, uint16_t buf_offset) :
        framebuf(width_, height_),
        size(width_ * (height_ / 8 + (height_ % 8 ? 1 : 0)) + buf_offset), stride(stride_), buffer_offset(buf_offset),
        buffer(std::shared_ptr<uint8_t>(new uint8_t[size])) {
    // zero out the buffer
    std::memset(buffer.get(), 0, size);
    if (stride < width_) stride = width_;
}

mono_vlsb::mono_vlsb(const uint8_t *image, uint8_t width_, uint16_t height_, uint16_t stride_, uint16_t buf_offset) :
        framebuf(width_, height_),
        size(width_ * (height_ / 8 + (height_ % 8 ? 1 : 0)) + buf_offset), stride(stride_), buffer_offset(buf_offset),
        buffer(std::shared_ptr<uint8_t>(new uint8_t[size])) {
    // copy image to the buffer
    std::memcpy(buffer.get() + buf_offset, image, size - buf_offset);
    if (stride < width) stride = width_;
}


void mono_vlsb::setpixel(uint16_t x, uint16_t y, uint32_t color) {
    size_t index = (y >> 3) * stride + x + buffer_offset;
    uint8_t offset = y & 0x07;
    buffer.get()[index] = (buffer.get()[index] & ~(0x01 << offset)) | ((color != 0) << offset);
}

uint32_t mono_vlsb::getpixel(uint16_t x, uint16_t y) const {
    return (buffer.get()[(y >> 3) * stride + x + buffer_offset] >> (y & 0x07)) & 0x01;
}

void mono_vlsb::fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    while (h--) {
        uint8_t *b = &buffer.get()[(y >> 3) * stride + x + buffer_offset];
        uint8_t offset = y & 0x07;
        for (unsigned int ww = w; ww; --ww) {
            *b = (*b & ~(0x01 << offset)) | ((color != 0) << offset);
            ++b;
        }
        ++y;
    }
}


