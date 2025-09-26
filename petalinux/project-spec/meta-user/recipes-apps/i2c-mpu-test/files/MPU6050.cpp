#include "MPU6050.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>
#include <fmt/core.h>
#include <fmt/format.h>

MPU6050::MPU6050(const std::string &devicePath, uint8_t deviceAddress)
    : m_I2CDevice(devicePath, deviceAddress)
{
    Initialize();
}

bool MPU6050::Initialize()
{
    if (!m_I2CDevice.IsOpen())
    {
        throw std::runtime_error("I2C device not open");
    }

    Reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    SetSleepMode(false);
    SetClockSource(0x01);
    SetSampleRate(7); // Sample rate = Gyroscope output rate / (1 + SMPLRT_DIV), Gyro output rate = 1kHz

    SetGyroScale(GyroScale::SCALE_250_DPS);
    SetAccelScale(AccelScale::SCALE_2G);

    return TestConnection();
}

void MPU6050::Reset()
{

    uint8_t resetCommand = 0x80;
    m_I2CDevice.WriteSingle(PWR_MGMT_1, resetCommand);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint8_t wakeCommand = 0x00;
    m_I2CDevice.WriteSingle(PWR_MGMT_1, wakeCommand);
}

bool MPU6050::TestConnection()
{
    uint8_t whoAmI = m_I2CDevice.ReadSingle(WHO_AM_I);
    if (whoAmI != m_I2CDevice.GetDeviceAddress())
    {
        throw std::runtime_error("MPU6050 connection failed: WHO_AM_I register mismatch");
    }
    return true;
}

void MPU6050::SetSleepMode(bool enable)
{
    uint8_t currentValue = m_I2CDevice.ReadSingle(PWR_MGMT_1);
    if (enable)
    {
        currentValue |= 0x40; // Set sleep bit
    }
    else
    {
        currentValue &= ~0x40; // Clear sleep bit
    }
    m_I2CDevice.WriteSingle(PWR_MGMT_1, currentValue);
}

void MPU6050::SetClockSource(uint8_t source)
{
    if (source > 7)
    {
        throw std::invalid_argument("Invalid clock source");
    }
    uint8_t currentValue = m_I2CDevice.ReadSingle(PWR_MGMT_1);
    currentValue = (currentValue & 0xF8) | (source & 0x07); // Clear and set clock source bits
    m_I2CDevice.WriteSingle(PWR_MGMT_1, currentValue);
}

void MPU6050::SetGyroScale(GyroScale scale)
{
    uint8_t currentValue = m_I2CDevice.ReadSingle(GYRO_CONFIG);
    currentValue = currentValue | static_cast<uint8_t>(scale); // Clear and set gyro scale bits
    m_I2CDevice.WriteSingle(GYRO_CONFIG, currentValue);
    m_GyroScale = scale;
}

void MPU6050::SetAccelScale(AccelScale scale)
{
    uint8_t currentValue = m_I2CDevice.ReadSingle(ACCEL_CONFIG);
    currentValue = currentValue | static_cast<uint8_t>(scale); // Clear and set accel scale bits
    m_I2CDevice.WriteSingle(ACCEL_CONFIG, currentValue);
    m_AccelScale = scale;
}

MPU6050::GyroScale MPU6050::GetGyroScale() const
{
    return m_GyroScale;
}

MPU6050::AccelScale MPU6050::GetAccelScale() const
{
    return m_AccelScale;
}

void MPU6050::SetSampleRate(uint8_t sample_rate_divider)
{
    m_I2CDevice.WriteSingle(SMPLRT_DIV, sample_rate_divider);
}

uint8_t MPU6050::GetSampleRate()
{
    uint8_t divider = m_I2CDevice.ReadSingle(SMPLRT_DIV);
    return divider;
}

float MPU6050::GetTemperatureCelsius()
{
    auto temp_raw = m_I2CDevice.ReadBurst(TEMP_OUT_H, 2);
    int16_t rawTemp = CombineBytes(temp_raw[0], temp_raw[1]);
    return (rawTemp / 340.0f) + 36.53f;
}

MPU6050::AccelData MPU6050::GetAccelData()
{
    auto accel_raw = m_I2CDevice.ReadBurst(ACCEL_XOUT_H, 6);
    int16_t rawX = CombineBytes(accel_raw[0], accel_raw[1]);
    int16_t rawY = CombineBytes(accel_raw[2], accel_raw[3]);
    int16_t rawZ = CombineBytes(accel_raw[4], accel_raw[5]);

    float scaleFactor = 0.0;
    switch (m_AccelScale)
    {
    case AccelScale::SCALE_2G:
        scaleFactor = 16384.0;
        break;
    case AccelScale::SCALE_4G:
        scaleFactor = 8192.0;
        break;
    case AccelScale::SCALE_8G:
        scaleFactor = 4096.0;
        break;
    case AccelScale::SCALE_16G:
        scaleFactor = 2048.0;
        break;
    }

    AccelData data;
    data.accel_x = rawX / scaleFactor;
    data.accel_y = rawY / scaleFactor;
    data.accel_z = rawZ / scaleFactor;

    return data;
}

MPU6050::GyroData MPU6050::GetGyroData()
{
    auto gyro_raw = m_I2CDevice.ReadBurst(GYRO_XOUT_H, 6);
    int16_t rawX = CombineBytes(gyro_raw[0], gyro_raw[1]);
    int16_t rawY = CombineBytes(gyro_raw[2], gyro_raw[3]);
    int16_t rawZ = CombineBytes(gyro_raw[4], gyro_raw[5]);

    float scaleFactor = 0.0;
    switch (m_GyroScale)
    {
    case GyroScale::SCALE_250_DPS:
        scaleFactor = 131.0;
        break;
    case GyroScale::SCALE_500_DPS:
        scaleFactor = 65.5;
        break;
    case GyroScale::SCALE_1000_DPS:
        scaleFactor = 32.8;
        break;
    case GyroScale::SCALE_2000_DPS:
        scaleFactor = 16.4;
        break;
    }

    GyroData data;
    data.gyro_x = rawX / scaleFactor;
    data.gyro_y = rawY / scaleFactor;
    data.gyro_z = rawZ / scaleFactor;

    return data;
}

void MPU6050::Enable_Fifo(uint8_t enable)
{
    uint8_t currentValue = m_I2CDevice.ReadSingle(USER_CTRL);
    if (enable)
    {
        currentValue |= 0x40; // Set FIFO_EN bit
    }
    else
    {
        currentValue &= ~0x40; // Clear FIFO_EN bit
    }
    m_I2CDevice.WriteSingle(USER_CTRL, currentValue);
}

uint16_t MPU6050::GetFIFOCount()
{
    auto fifo_count_raw = m_I2CDevice.ReadBurst(FIFO_COUNT_H, 2);
    return static_cast<uint16_t>((fifo_count_raw[0] << 8) | fifo_count_raw[1]);
}

int16_t MPU6050::CombineBytes(uint8_t high, uint8_t low) const
{
    return static_cast<int16_t>((high << 8) | low);
}

void MPU6050::PrintValues()
{
    auto temp = GetTemperatureCelsius();
    auto accel = GetAccelData();
    auto gyro = GetGyroData();
    auto fifoCount = GetFIFOCount();
    fmt::print("----------------------------------------------------------\n");
    fmt::print("FIFO Count: {}\n", fifoCount);
    fmt::print("Temperature: {:.2f} °C\n", temp);
    fmt::print("Accelerometer - X: {:.2f} g, Y: {:.2f} g, Z: {:.2f} g\n", accel.accel_x, accel.accel_y, accel.accel_z);
    fmt::print("Gyroscope - X: {:.2f} °/s, Y: {:.2f} °/s, Z: {:.2f} °/s\n", gyro.gyro_x, gyro.gyro_y, gyro.gyro_z);
    fmt::print("----------------------------------------------------------\n");
}