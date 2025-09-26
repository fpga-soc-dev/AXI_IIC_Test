#include <fmt/core.h>
#include <fmt/format.h>
#include <thread>
#include <chrono>
#include "MPU6050.hpp"

int main()
{
    const std::string devicePath = "/dev/i2c-0";
    const uint8_t deviceAddress = 0x68;

    try
    {
        MPU6050 mpu(devicePath, deviceAddress);

        mpu.Reset();
        fmt::print("MPU6050 reset successfully.\n");

        if (mpu.TestConnection())
        {
            fmt::print("MPU6050 connection successful.\n");
        }

        while (true)
        {
            mpu.PrintValues();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &ex)
    {
        fmt::print("Error: {}\n", ex.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}