//
// Created by Keijo Länsikunnas on 18.2.2024.
//
// based on Pico examples: ssd1306.c and
// MicroPython SSD1306 library

#include "ssd1306.h"


// commands (see datasheet)
#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

/* Constructor allocates buffer that is one bigger than what is needed.
 * The extra byte is needed for the command byte when updating the display.
 * Height must be multiple of 8.
 */
ssd1306::ssd1306(i2c_inst *i2c, uint16_t device_address, uint16_t width, uint16_t height) :
        mono_vlsb(width, height, width, 1),
        ssd1306_i2c(i2c), address(device_address) {
    // set control byte at the beginning of frame buffer
    buffer.get()[0] = 0x40;
    init();
}

void ssd1306::init() {
    // Some of these commands are not strictly necessary as the reset
    // process defaults to some of these but they are shown here
    // to demonstrate what the initialization sequence looks like
    // Some configuration values are recommended by the board manufacturer

    uint8_t cmds[] = {
            SSD1306_SET_DISP,               // set display off
            /* memory mapping */
            SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
            0x00,                           // horizontal addressing mode
            /* resolution and layout */
            SSD1306_SET_DISP_START_LINE,    // set display start line to 0
            SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
            SSD1306_SET_MUX_RATIO,          // set multiplex ratio
            uint8_t (height - 1),             // Display height - 1
            SSD1306_SET_COM_OUT_DIR |
            0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
            SSD1306_SET_DISP_OFFSET,        // set display offset
            0x00,                           // no offset
            SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number.
            // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
            0x02, // this is changed later in the constructor if display size is not 128x32

            /* timing and driving scheme */
            SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
            0x80,                           // div ratio of 1, standard freq
            SSD1306_SET_PRECHARGE,          // set pre-charge period
            0xF1,                           // Vcc internally generated on our board
            SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
            0x30,                           // 0.83xVcc
            /* display */
            SSD1306_SET_CONTRAST,           // set contrast control
            0xFF,
            SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
            SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
            SSD1306_SET_CHARGE_PUMP,        // set charge pump
            0x14,                           // Vcc internally generated on our board
            SSD1306_SET_SCROLL |
            0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
            SSD1306_SET_DISP | 0x01, // turn display on
    };
    if(height > 32) cmds[11] = 0x12;

    for(auto value : cmds) {
        send_cmd(value);
    }

}


void ssd1306::send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(ssd1306_i2c, address, buf, 2, false);
}

void ssd1306::show() {
    uint16_t x0 = 0;
    uint16_t x1 = width - 1;
    if(width != 128) {
        // narrow displays use centred columns
        uint16_t col_offset = (128 - width); // 2
        x0 += col_offset;
        x1 += col_offset;
    }
    send_cmd(SSD1306_SET_COL_ADDR);
    send_cmd(x0);
    send_cmd(x1);
    send_cmd(SSD1306_SET_PAGE_ADDR);
    send_cmd(0);
    send_cmd(height / 8 - 1); // nr of pages??
    // set control byte at the beginning of frame buffer
    buffer.get()[0] = 0x40;
    // write the frame buffer at one go
    i2c_write_blocking(ssd1306_i2c, address, buffer.get(), size, false);
}
