//
// Created by Keijo Länsikunnas on 10.9.2024.
//
#include <string>
#include "pico/stdlib.h"
#include "blinker.h"
#include "FreeRTOS.h"
#include "task.h"

Blinker::Blinker(int led_) : name("blink" + std::to_string(led_)), led(led_), delay(300) {
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);
    gpio_put(led, false);
    xTaskCreate(Blinker::runner, name.c_str(), 256, (void *) this,
                tskIDLE_PRIORITY + 1, &handle);
}

void Blinker::off() {
    xTaskNotify(handle, 0xFFFFFFFF, eSetValueWithOverwrite);
}

void Blinker::on(uint32_t delay) {
    xTaskNotify(handle, delay, eSetValueWithOverwrite);
}

void Blinker::run() {
    bool on = false;
    while (true) {
        uint32_t cmd = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(delay));
        switch (cmd) {
            case 0:
                // timed out --> do nothing
                break;
            case 0xFFFFFFFF:
                on = false;
                break;
            default:
                delay = cmd;
                on = true;
                break;
        }
        if (on) {
            gpio_put(led, !gpio_get_out_level(led));
        } else {
            gpio_put(led, false);
        }
    }
}

void Blinker::runner(void *params) {
    Blinker *instance = static_cast<Blinker *>(params);
    instance->run();
}

