#pragma once
#include "pch.h"
#include <modbus.h>
#include <mutex>
#include <string>
#include <memory>

// ModbusManager: thread-safe thin wrapper around libmodbus.
// Implementation moved inline to header to avoid linker missing-symbol issues
// when this translation unit is not linked into the main project.

class ModbusManager {
public:
    ModbusManager() : ctx_(nullptr) {}
    ~ModbusManager() { Close(); }

    // 禁止複製
    ModbusManager(const ModbusManager&) = delete;
    ModbusManager& operator=(const ModbusManager&) = delete;

    // 初始化連線
    bool Initialize(const std::string& ip, int port, int station_id)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (ctx_) {
            Close();
        }
        ctx_ = modbus_new_tcp(ip.c_str(), port);
        if (!ctx_) return false;
        if (modbus_set_slave(ctx_, station_id) != 0) {
            modbus_free(ctx_);
            ctx_ = nullptr;
            return false;
        }
        if (modbus_connect(ctx_) == -1) {
            modbus_free(ctx_);
            ctx_ = nullptr;
            return false;
        }
        return true;
    }

    // 關閉連線
    void Close()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (ctx_) {
            modbus_close(ctx_);
            modbus_free(ctx_);
            ctx_ = nullptr;
        }
    }

    // Thread-safe 讀/寫操作
    int ReadHoldingRegisters(int addr, int nb, uint16_t* dest)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!ctx_) return -1;
        return modbus_read_registers(ctx_, addr, nb, dest);
    }

    int WriteHoldingRegisters(int addr, int nb, const uint16_t* data)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!ctx_) return -1;
        return modbus_write_registers(ctx_, addr, nb, data);
    }

    int WriteHoldingRegister(int addr, uint16_t value)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!ctx_) return -1;
        return modbus_write_register(ctx_, addr, value);
    }

    int ReadBits(int addr, int nb, uint8_t* dest)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!ctx_) return -1;
        return modbus_read_bits(ctx_, addr, nb, dest);
    }

    int WriteBit(int addr, int status)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!ctx_) return -1;
        return modbus_write_bit(ctx_, addr, status);
    }

private:
    modbus_t* ctx_;
    std::mutex mtx_;
};