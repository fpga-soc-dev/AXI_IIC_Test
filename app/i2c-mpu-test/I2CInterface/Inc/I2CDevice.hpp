#pragma once

#include "FileHandle.hpp"
#include <span>
#include <vector>
#include <mutex>
#include <memory>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>

namespace I2C
{
    enum class Error
    {
        OK = 0,
        NOT_OPEN,
        OPEN_FAILED,
        IOCTL_FAILED,
        POLL_TIMEOUT,
        INVALID_ARGUMENT,
    };

    struct Message
    {
        uint16_t address = 0;
        uint16_t flags = 0;
        size_t length = 0;
        std::span<uint8_t> data;
    };

    struct Options
    {
        int timeout_ms = 1000;
        int retries = 2;
    };

    class I2CDevice
    {
    public:
        I2CDevice(std::string device_path, uint16_t device_address);

        I2CDevice(const I2CDevice &) = delete;
        I2CDevice &operator=(const I2CDevice &) = delete;
        I2CDevice(I2CDevice &&) noexcept;
        I2CDevice &operator=(I2CDevice &&) noexcept;
        ~I2CDevice() = default;

        uint8_t ReadSingle(uint8_t reg);
        void WriteSingle(uint8_t reg, uint8_t value);
        std::vector<uint8_t> ReadBurst(uint8_t reg, size_t length);
        void WriteBurst(uint8_t reg, const std::span<const uint8_t> &data);

        bool IsOpen() const;
        std::string GetDevicePath() const;
        uint16_t GetDeviceAddress() const;

    private:
        std::string m_DevicePath;
        uint16_t m_DeviceAddress;
        std::unique_ptr<Utils::FileHandle> m_FileHandle;
        mutable std::mutex m_Mutex;

        Error WaitForDevice(int timeout_ms);
        Error Transfer(std::span<Message> messages, const Options &options = {});
    };
}