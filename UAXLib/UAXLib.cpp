#include "pch.h"  
#include "UAXLib.h"
//#include "UModbusClient.h"
#include <stdexcept>
#include <errno.h>

UModbusClient::UModbusClient() : ctx_(nullptr), port_(0), station_id_(0) {
}

UModbusClient::~UModbusClient() {
    Close();
}

bool UModbusClient::Initialize(const std::string& ip_address, int port, uint8_t station_id) {
    // Store parameters
    ip_address_ = ip_address;
    port_ = port;
    station_id_ = station_id;

    // Initialize Modbus TCP context
    ctx_ = modbus_new_tcp(ip_address.c_str(), port);
    if (ctx_ == nullptr) {
        return false;
    }

    // Set slave ID
    if (modbus_set_slave(ctx_, station_id) != 0) {
        modbus_free(ctx_);
        ctx_ = nullptr;
        return false;
    }

    // Connect to the server
    if (modbus_connect(ctx_) == -1) {
        modbus_free(ctx_);
        ctx_ = nullptr;
        return false;
    }

    return true;
}

int UModbusClient::ReadDiscrete(uint16_t address, uint16_t quantity, uint8_t* dest) {
    if (!ctx_) {
        return -1;
    }

    // Read discrete inputs
    int rc = modbus_read_input_bits(ctx_, address, quantity, dest);
    if (rc == -1) {
        return -errno;
    }

    return rc; // Returns number of discrete inputs read
}

int UModbusClient::ReadHoldingRegisters(uint16_t address, uint16_t quantity, uint16_t* dest) {
    if (!ctx_) {
        return -1;
    }

    // Read holding registers
    int rc = modbus_read_registers(ctx_, address, quantity, dest);
    if (rc == -1) {
        return -errno;
    }

    return rc; // Returns number of registers read
}

int UModbusClient::WriteDiscrete(uint16_t address, bool value) {
    if (!ctx_) {
        return -1;
    }

    // Write single coil
    int rc = modbus_write_bit(ctx_, address, value ? 1 : 0);
    if (rc == -1) {
        return -errno;
    }

    return rc; // Returns 1 on success
}

int UModbusClient::WriteHoldingRegisters(uint16_t address, uint16_t quantity, const uint16_t* values) {
    if (!ctx_) {
        return -1;
    }

    // Write multiple registers
    int rc = modbus_write_registers(ctx_, address, quantity, values);
    if (rc == -1) {
        return -errno;
    }

    return rc; // Returns number of registers written
}

void UModbusClient::Close() {
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
    }
}
