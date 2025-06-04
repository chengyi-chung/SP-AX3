#pragma once

#pragma once

#ifdef UMODBUSCLIENT_EXPORTS
#define UMODBUSCLIENT_API __declspec(dllexport)
#else
#define UMODBUSCLIENT_API __declspec(dllimport)
#endif

#include <modbus.h>
#include <string>

class UMODBUSCLIENT_API UModbusClient {
public:
    // Constructor
    UModbusClient();

    // Destructor
    ~UModbusClient();

    // Initialize: Input IP address, port, and station ID
    bool Initialize(const std::string& ip_address, int port, uint8_t station_id);

    // Read Discrete Inputs
    int ReadDiscrete(uint16_t address, uint16_t quantity, uint8_t* dest);

    // Read Holding Registers
    int ReadHoldingRegisters(uint16_t address, uint16_t quantity, uint16_t* dest);

    // Write Single Discrete (Coil)
    int WriteDiscrete(uint16_t address, bool value);

    // Write Multiple Holding Registers
    int WriteHoldingRegisters(uint16_t address, uint16_t quantity, const uint16_t* values);

    // Close connection and cleanup
    void Close();

private:
    modbus_t* ctx_;              // libmodbus context
    std::string ip_address_;     // IP address of the Modbus server
    int port_;                   // Port number
    uint8_t station_id_;         // Modbus slave ID
};