#pragma once

#include "I2CDevice.hpp"

class MPU6050
{
public:
    // MPU6050 register addresses
    static constexpr uint8_t PWR_MGMT_1 = 0x6B;
    static constexpr uint8_t PWR_MGMT_2 = 0x6C;
    static constexpr uint8_t SMPLRT_DIV = 0x19;
    static constexpr uint8_t CONFIG = 0x1A;
    static constexpr uint8_t GYRO_CONFIG = 0x1B;
    static constexpr uint8_t ACCEL_CONFIG = 0x1C;
    static constexpr uint8_t WHO_AM_I = 0x75;
    static constexpr uint8_t FIFO_EN = 0x23;
    static constexpr uint8_t FIFO_COUNT_H = 0x72;
    static constexpr uint8_t FIFO_COUNT_L = 0x73;

    // Data registers
    static constexpr uint8_t ACCEL_XOUT_H = 0x3B;
    static constexpr uint8_t ACCEL_XOUT_L = 0x3C;
    static constexpr uint8_t ACCEL_YOUT_H = 0x3D;
    static constexpr uint8_t ACCEL_YOUT_L = 0x3E;
    static constexpr uint8_t ACCEL_ZOUT_H = 0x3F;
    static constexpr uint8_t ACCEL_ZOUT_L = 0x40;
    static constexpr uint8_t TEMP_OUT_H = 0x41;
    static constexpr uint8_t TEMP_OUT_L = 0x42;
    static constexpr uint8_t GYRO_XOUT_H = 0x43;
    static constexpr uint8_t GYRO_XOUT_L = 0x44;
    static constexpr uint8_t GYRO_YOUT_H = 0x45;
    static constexpr uint8_t GYRO_YOUT_L = 0x46;
    static constexpr uint8_t GYRO_ZOUT_H = 0x47;
    static constexpr uint8_t GYRO_ZOUT_L = 0x48;

    // Configuration values
    enum class GyroScale : uint8_t
    {
        SCALE_250_DPS = 0x00,
        SCALE_500_DPS = 0x08,
        SCALE_1000_DPS = 0x10,
        SCALE_2000_DPS = 0x18
    };

    enum class AccelScale : uint8_t
    {
        SCALE_2G = 0x00,
        SCALE_4G = 0x08,
        SCALE_8G = 0x10,
        SCALE_16G = 0x18
    };

    struct AccelData
    {
        float accel_x, accel_y, accel_z;
    };

    struct GyroData
    {
        float gyro_x, gyro_y, gyro_z;
    };

    MPU6050(const std::string &devicePath, uint8_t deviceAddress);
    ~MPU6050() = default;

    bool Initialize();
    void Reset();
    bool TestConnection();

    void SetSleepMode(bool enable);

    void SetClockSource(uint8_t source);
    void SetGyroScale(GyroScale scale);
    MPU6050::GyroScale GetGyroScale() const;

    void SetAccelScale(AccelScale scale);
    MPU6050::AccelScale GetAccelScale() const;

    void SetSampleRate(uint8_t sample_rate_divider);
    uint8_t GetSampleRate();

    float GetTemperatureCelsius();
    AccelData GetAccelData();
    GyroData GetGyroData();

    void Enable_Fifo(uint8_t config);
    uint16_t GetFIFOCount();

    void PrintValues();

private:
    I2C::I2CDevice m_I2CDevice;
    GyroScale m_GyroScale{GyroScale::SCALE_250_DPS};
    AccelScale m_AccelScale{AccelScale::SCALE_2G};

    int16_t CombineBytes(uint8_t high, uint8_t low) const;
};