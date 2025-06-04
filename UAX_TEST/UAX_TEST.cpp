#include "..\UAXLib\UAXLib.h"
#include <iostream>

//Add UAXLib.lib  file to the project directory  //debug\\UAXLib.lib
#ifdef _DEBUG
#pragma comment(lib,"..\\x64\\debug\\UAXLib.lib")                  
#else
#pragma comment(lib,"..\\x64\\release\\UAXLib.lib") 
#endif

int main() {
    try {
        // 初始化客戶端
        UModbusClient client;
        if (!client.Initialize("192.168.1.100", 502, 1)) {
            std::cerr << "初始化 Modbus 客戶端失敗" << std::endl;
            return -1;
        }

        // 讀取離散輸入
        uint8_t discrete_inputs[10];
        int rc = client.ReadDiscrete(0, 10, discrete_inputs);
        if (rc > 0) {
            std::cout << "讀取 " << rc << " 個離散輸入" << std::endl;
        }
        else {
            std::cerr << "讀取離散輸入失敗: " << rc << std::endl;
        }

        // 讀取保持寄存器
        uint16_t registers[5];
        rc = client.ReadHoldingRegisters(100, 5, registers);
        if (rc > 0) {
            std::cout << "讀取 " << rc << " 個保持寄存器" << std::endl;
        }
        else {
            std::cerr << "讀取寄存器失敗: " << rc << std::endl;
        }

        // 寫入單個線圈
        rc = client.WriteDiscrete(0, true);
        if (rc > 0) {
            std::cout << "成功寫入線圈" << std::endl;
        }
        else {
            std::cerr << "寫入離散失敗: " << rc << std::endl;
        }

        // 寫入多個寄存器
        uint16_t values[] = { 100, 200, 300 };
        rc = client.WriteHoldingRegisters(200, 3, values);
        if (rc > 0) {
            std::cout << "寫入 " << rc << " 個寄存器" << std::endl;
        }
        else {
            std::cerr << "寫入寄存器失敗: " << rc << std::endl;
        }

        // 關閉客戶端
        client.Close();
    }
    catch (const std::exception& e) {
        std::cerr << "例外: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}