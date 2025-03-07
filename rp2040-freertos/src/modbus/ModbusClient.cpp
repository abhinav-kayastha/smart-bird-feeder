//
// Created by Keijo Länsikunnas on 14.2.2024.
//

#include "ModbusClient.h"
#include "pico/time.h"

ModbusClient::ModbusClient(std::shared_ptr<PicoOsUart> uart_) : uart(uart_) {
    platform_conf.transport = NMBS_TRANSPORT_RTU;
    platform_conf.read = uart_transport_read;
    platform_conf.write = uart_transport_write;
    platform_conf.arg = (void *) uart.get();    // Passing our uart handle to the read/write functions

    // Create the modbus client
    nmbs_error err = nmbs_client_create(&nmbs, &platform_conf);
    if (err != NMBS_ERROR_NONE) {
        // throw exception??
    }
    nmbs_set_destination_rtu_address(&nmbs, 1); //default value that will be updated later
    // Set only the response timeout.
    nmbs_set_read_timeout(&nmbs, 1000);
    // set byte timeout. Standard says 1.5 x byte time between chars and 3.5 x byte time to end frame
    // so we choose 3 x byte time --> 3 ms @ 9600bps
    nmbs_set_byte_timeout(&nmbs, 3);


}

int32_t ModbusClient::uart_transport_read(uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg) {
    auto uart = static_cast<PicoOsUart *>(arg);
    uint32_t timeout = byte_timeout_ms < 0 ? portMAX_DELAY : byte_timeout_ms;
    uint flv = uart->get_fifo_level();
    if (flv) {
        // The timeout must be atleast fifo level x bits/ch x bit time.
        // Minimum fifo level is 4.
        // There is also fifo inactivity timeout which is fixed at 32 bit time
        // Worst case delay is fifo level - 1 + inactivity timeout
        // To play safe we set it to fifo level + inactivity timeout
        uint64_t fifo_to = ((flv * 10 + 32) * 1000000ULL) / uart->get_baud();
        // us --> ms, rounding up
        fifo_to = fifo_to / 1000 + (fifo_to % 1000 ? 1 : 0);
        // if delay caused by fifo is longer than requested byte timeout
        // then use the fifo timeout
        if (timeout < fifo_to) timeout = fifo_to;
    }
    // debug printout
    //printf("flv=%u, bto=%d, cnt=%d, to=%u\n",flv,byte_timeout_ms,count, (uint) timeout);
    int32_t rcnt = 0;
    int32_t cnt = 0;
    do  {
        //gpio_put(DBG_PIN1, true);
        cnt = uart->read(buf + rcnt, count - rcnt, timeout);
        rcnt += cnt;
    } while( rcnt < count && cnt > 0 );
    //gpio_put(DBG_PIN1, false);

    return rcnt;
}

int32_t ModbusClient::uart_transport_write(const uint8_t *buf, uint16_t count, int32_t byte_timeout_ms, void *arg) {
    return static_cast<PicoOsUart *>(arg)->write(buf, count, byte_timeout_ms);
}

void ModbusClient::set_destination_rtu_address(uint8_t address) {
    nmbs_set_destination_rtu_address(&nmbs, address);
}

nmbs_error ModbusClient::read_coils(uint16_t address, uint16_t quantity, nmbs_bitfield coils_out) {
    return nmbs_read_coils(&nmbs,address,quantity,coils_out);
}

nmbs_error ModbusClient::read_discrete_inputs(uint16_t address, uint16_t quantity, nmbs_bitfield inputs_out) {
    return nmbs_read_discrete_inputs(&nmbs, address, quantity, inputs_out);
}

nmbs_error ModbusClient::read_holding_registers(uint16_t address, uint16_t quantity, uint16_t *registers_out) {
    return nmbs_read_holding_registers(&nmbs, address, quantity, registers_out);
}

nmbs_error ModbusClient::read_input_registers(uint16_t address, uint16_t quantity, uint16_t *registers_out) {
    return nmbs_read_input_registers(&nmbs, address, quantity, registers_out);
}

nmbs_error ModbusClient::write_single_coil(uint16_t address, bool value) {
    return nmbs_write_single_coil(&nmbs, address, value);
}

nmbs_error ModbusClient::write_single_register(uint16_t address, uint16_t value) {
    return nmbs_write_single_register(&nmbs, address, value);
}

nmbs_error ModbusClient::write_multiple_coils(uint16_t address, uint16_t quantity, const nmbs_bitfield coils) {
    return nmbs_write_multiple_coils(&nmbs, address, quantity, coils);
}

nmbs_error ModbusClient::write_multiple_registers(uint16_t address, uint16_t quantity, const uint16_t *registers) {
    return nmbs_write_multiple_registers(&nmbs, address, quantity, registers);
}
