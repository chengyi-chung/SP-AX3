#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <locale>
#include <string>
#include <vector>

struct ModbusPathWriteResult {
    int returned = -2; // Not attempted; -1 is a Modbus failure.
    int errorCode = 0;
    std::string error;
};

inline std::string ModbusCsvQuote(const std::string& value) {
    std::string quoted = "\"";
    for (char c : value) { if (c == '"') quoted += '"'; quoted += c; }
    return quoted + '"';
}

// Raw words come from the actual write buffers, including clamping and padding.
// RESULT is the client API outcome, not a PLC read-back or motion acknowledgement.
inline bool AppendModbusPathCsv(const std::string& file, const std::string& batch,
    const std::string& timestamp, const std::string& phase, const std::string& ip,
    int port, int station, size_t validPoints,
    const std::array<std::vector<uint16_t>, 3>& buffers,
    const std::array<ModbusPathWriteResult, 3>& results)
{
    std::ofstream out(file, std::ios::app | std::ios::binary);
    if (!out) return false;
    out.imbue(std::locale::classic());
    out.seekp(0, std::ios::end);
    if (out.tellp() == 0) {
        out << "timestamp_local,batch_id,phase,ip,port,unit_id,function_code,axis,address_zero_based,point_1based,padded,raw_uint16,raw_hex,signed_int16,command_mm,status,returned_count,errno,error\r\n";
    }
    const char* axes[] = { "X1", "Y", "X2" };
    const int starts[] = { 14, 44, 74 };
    for (size_t axis = 0; axis < buffers.size(); ++axis) {
        const auto& r = results[axis];
        const char* status = phase == "PREPARED" ? "PREPARED" :
            r.returned == -2 ? "NOT_ATTEMPTED" :
            r.returned == static_cast<int>(buffers[axis].size()) ? "SUCCESS" : "FAILED";
        for (size_t i = 0; i < buffers[axis].size(); ++i) {
            const auto raw = buffers[axis][i];
            const int signedValue = raw < 32768 ? raw : static_cast<int>(raw) - 65536;
            out << ModbusCsvQuote(timestamp) << ',' << ModbusCsvQuote(batch) << ','
                << phase << ',' << ModbusCsvQuote(ip) << ',' << port << ',' << station
                << ",16," << axes[axis] << ',' << starts[axis] + i << ',' << i + 1
                << ',' << (i >= validPoints ? 1 : 0) << ',' << raw << ",0x"
                << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << raw
                << std::dec << std::setfill(' ') << ',' << signedValue << ','
                << std::fixed << std::setprecision(1) << signedValue / 10.0 << ','
                << status << ',' << r.returned << ',' << r.errorCode << ','
                << ModbusCsvQuote(r.error) << "\r\n";
        }
    }
    out.flush();
    return out.good();
}
