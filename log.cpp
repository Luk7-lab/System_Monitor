#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <cstdio>

struct SystemUsage {
    float cpu;
    float ram;
};

SystemUsage getSystemUsage() {
    float cpuLoad;
    FILE* pipe = popen("top -bn1 | grep 'Cpu(s)' | awk '{print 100 - $8}'", "r");
    if (!pipe) return {0, 0};
    fscanf(pipe, "%f", &cpuLoad);
    pclose(pipe);

    float ramUsage;
    pipe = popen("free | grep Mem | awk '{print $3/$2 * 100.0}'", "r");
    if (!pipe) return {cpuLoad, 0};
    fscanf(pipe, "%f", &ramUsage);
    pclose(pipe);

    return {cpuLoad, ramUsage};
}

void logToCSV(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    file << "Timestamp,CPU Usage (%),RAM Usage (%)\n";

    while (true) {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        SystemUsage usage = getSystemUsage();
        
        file << std::ctime(&now) << "," << usage.cpu << "," << usage.ram << "\n";
        file.flush();

        std::this_thread::sleep_for(std::chrono::minutes(2));
        file.seekp(0);
        file << "Timestamp,CPU Usage (%),RAM Usage (%)\n";
    }
}

int main() {
    logToCSV("system_usage_log.csv");
    return 0;
}
