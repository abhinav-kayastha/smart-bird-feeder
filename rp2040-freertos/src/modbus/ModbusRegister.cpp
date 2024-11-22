//
// Created by Keijo Länsikunnas on 14.2.2024.
//

#include "ModbusRegister.h"

ModbusRegister::ModbusRegister(std::shared_ptr<ModbusClient> client_, int server_address, int register_address,
                               bool holding_register) :
        client(client_), server(server_address), reg_addr(register_address), hr(holding_register) {

}

uint16_t ModbusRegister::read() {
    uint16_t value = 0;
    // With RTU one client handles all devices (servers) on the same bus
    // so we need to set the server address
    client->set_destination_rtu_address(server);
    if(hr) client->read_holding_registers(reg_addr, 1, &value);
    else client->read_input_registers(reg_addr, 1, &value);
    return value;
}

void ModbusRegister::write(uint16_t value) {
    // only holding register is writable
    if(hr){
        // With RTU one client handles all devices (servers) on the same bus
        // so we need to set the server address
        client->set_destination_rtu_address(server);
        client->write_single_register(reg_addr, value);
    }
}
