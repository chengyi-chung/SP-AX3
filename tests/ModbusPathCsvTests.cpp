#include "../ModbusPathCsv.h"
#include <cstdio>
#include <stdexcept>

int main() {
    const std::string file = "tmp/inset-tests/modbus_csv_test.csv";
    std::remove(file.c_str());
    std::array<std::vector<uint16_t>, 3> buffers;
    for (auto& axis : buffers) axis.assign(30, 65526); // -10 units = -1.0 mm
    std::array<ModbusPathWriteResult, 3> results;
    if (!AppendModbusPathCsv(file, "test-batch", "2026-10-05 12:00:00.000", "PREPARED",
        "127.0.0.1", 502, 1, 25, buffers, results)) return 1;
    results[0].returned = 30;
    results[1] = { -1, 5, "test, \"failure\"" };
    if (!AppendModbusPathCsv(file, "test-batch", "2026-10-05 12:00:00.000", "RESULT",
        "127.0.0.1", 502, 1, 25, buffers, results)) return 2;
    std::ifstream input(file);
    std::string line;
    int lines = 0;
    while (std::getline(input, line)) ++lines;
    if (lines != 181) throw std::runtime_error("wrong row count or duplicate header");
}
