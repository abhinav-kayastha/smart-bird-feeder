//
// Created by Keijo Länsikunnas on 14.2.2024.
//

#ifndef UART_IRQ_MODBUSREGISTER_H
#define UART_IRQ_MODBUSREGISTER_H

#include <memory>
#include "ModbusClient.h"

class ModbusRegister {
public:
    ModbusRegister(std::shared_ptr<ModbusClient> client_, int server_address, int register_address, bool holding_register = true);
    uint16_t read();
    void write(uint16_t value);
private:
    std::shared_ptr<ModbusClient> client;
    int server;
    int reg_addr;
    bool hr;

};


#endif //UART_IRQ_MODBUSREGISTER_H
