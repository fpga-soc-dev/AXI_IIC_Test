#include "I2CDevice.hpp"

#include <thread>
#include <stdexcept>
#include <system_error>

namespace I2C
{
    I2CDevice::I2CDevice(std::string device_path, uint16_t device_address)
        : m_DevicePath(std::move(device_path)), m_DeviceAddress(device_address)
    {
        if (device_address > 0x7F)
        {
            throw std::invalid_argument("Invalid I2C device address");
        }

        try
        {
            m_FileHandle = std::make_unique<Utils::FileHandle>(m_DevicePath, O_RDWR | O_CLOEXEC);
        }
        catch (const std::system_error &e)
        {
            throw std::runtime_error("Failed to open I2C device: " + m_DevicePath + " - " + e.what());
        }

        if (!m_FileHandle->IsOpen())
        {
            throw std::runtime_error("Failed to open I2C device: " + m_DevicePath);
        }
    }

    I2CDevice::I2CDevice(I2CDevice &&other) noexcept
        : m_DevicePath(std::move(other.m_DevicePath)),
          m_DeviceAddress(other.m_DeviceAddress),
          m_FileHandle(std::move(other.m_FileHandle))
    {
    }

    I2CDevice &I2CDevice::operator=(I2CDevice &&other) noexcept
    {
        if (this != &other)
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_FileHandle = std::move(other.m_FileHandle);
            m_DevicePath = std::move(other.m_DevicePath);
            m_DeviceAddress = other.m_DeviceAddress;
        }
        return *this;
    }

    bool I2CDevice::IsOpen() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_FileHandle && m_FileHandle->IsOpen();
    }

    std::string I2CDevice::GetDevicePath() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_DevicePath;
    }

    uint16_t I2CDevice::GetDeviceAddress() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_DeviceAddress;
    }

    Error I2CDevice::WaitForDevice(int timeout_ms)
    {
        struct pollfd pfd{};
        pfd.fd = m_FileHandle->GetFd();
        pfd.events = POLLIN | POLLOUT;

        int ret = ::poll(&pfd, 1, timeout_ms);
        if (ret < 0)
        {
            return Error::IOCTL_FAILED;
        }
        else if (ret == 0)
        {
            return Error::POLL_TIMEOUT;
        }
        return Error::OK;
    }

    Error I2CDevice::Transfer(std::span<Message> messages, const Options &options)
    {
        if (messages.empty())
        {
            return Error::INVALID_ARGUMENT;
        }

        std::lock_guard<std::mutex> lock(m_Mutex);

        if (!m_FileHandle || !m_FileHandle->IsOpen())
        {
            return Error::NOT_OPEN;
        }

        if (options.timeout_ms > 0)
        {
            auto timeout_error = WaitForDevice(options.timeout_ms);
            if (timeout_error != Error::OK)
            {
                return timeout_error;
            }
        }

        std::vector<struct i2c_msg> i2c_msgs;
        i2c_msgs.reserve(messages.size());

        for (const auto &msg : messages)
        {
            struct i2c_msg i2c_msg{};
            i2c_msg.addr = msg.address;
            i2c_msg.flags = msg.flags;
            i2c_msg.len = static_cast<__u16>(msg.length);
            i2c_msg.buf = msg.data.data();
            i2c_msgs.push_back(i2c_msg);
        }

        struct i2c_rdwr_ioctl_data ioctl_data{};
        ioctl_data.msgs = i2c_msgs.data();
        ioctl_data.nmsgs = static_cast<__u32>(i2c_msgs.size());

        for (int attempt = 0; attempt <= options.retries; ++attempt)
        {
            if (::ioctl(m_FileHandle->GetFd(), I2C_RDWR, &ioctl_data) < 0)
            {
                if (attempt == options.retries)
                {
                    return Error::IOCTL_FAILED;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            return Error::OK;
        }

        return Error::IOCTL_FAILED;
    }

    uint8_t I2CDevice::ReadSingle(uint8_t reg)
    {
        std::array<uint8_t, 1> data{};
        std::array<uint8_t, 1> reg_data = {reg};
        I2C::Message messages[2] = {};

        messages[0].address = m_DeviceAddress;
        messages[0].flags = 0; // Write
        messages[0].length = 1;
        messages[0].data = std::span<uint8_t>(reg_data.data(), reg_data.size());

        messages[1].address = m_DeviceAddress;
        messages[1].flags = I2C_M_RD; // Read
        messages[1].length = 1;
        messages[1].data = std::span<uint8_t>(data.data(), data.size());

        I2C::Options options{};
        options.timeout_ms = 1000;
        options.retries = 2;

        auto error = Transfer(std::span<I2C::Message>(messages, 2), options);
        if (error != I2C::Error::OK)
        {
            throw std::runtime_error("I2C Transfer failed with error code: " + std::to_string(static_cast<int>(error)));
        }

        return data[0];
    }

    void I2CDevice::WriteSingle(uint8_t reg, uint8_t value)
    {
        std::array<uint8_t, 2> data = {reg, value};
        I2C::Message message = {};

        message.address = m_DeviceAddress;
        message.flags = 0; // Write
        message.length = data.size();
        message.data = std::span<uint8_t>(data.data(), data.size());

        I2C::Options options{};
        options.timeout_ms = 1000;
        options.retries = 2;

        auto error = Transfer(std::span<I2C::Message>(&message, 1), options);
        if (error != I2C::Error::OK)
        {
            throw std::runtime_error("I2C Transfer failed with error code: " + std::to_string(static_cast<int>(error)));
        }
    }

    std::vector<uint8_t> I2CDevice::ReadBurst(uint8_t reg, size_t length)
    {
        if (length == 0)
        {
            throw std::invalid_argument("Length must be greater than zero");
        }

        std::vector<uint8_t> data(length);
        std::array<uint8_t, 1> reg_data = {reg};
        I2C::Message messages[2] = {};

        messages[0].address = m_DeviceAddress;
        messages[0].flags = 0; // Write
        messages[0].length = 1;
        messages[0].data = std::span<uint8_t>(reg_data.data(), reg_data.size());

        messages[1].address = m_DeviceAddress;
        messages[1].flags = I2C_M_RD; // Read
        messages[1].length = length;
        messages[1].data = std::span<uint8_t>(data.data(), data.size());

        I2C::Options options{};
        options.timeout_ms = 1000;
        options.retries = 2;

        auto error = Transfer(std::span<I2C::Message>(messages, 2), options);
        if (error != I2C::Error::OK)
        {
            throw std::runtime_error("I2C Transfer failed with error code: " + std::to_string(static_cast<int>(error)));
        }

        return data;
    }

    void I2CDevice::WriteBurst(uint8_t reg, const std::span<const uint8_t> &data)
    {
        if (data.empty())
        {
            throw std::invalid_argument("Data span must not be empty");
        }

        std::vector<uint8_t> buffer(1 + data.size());
        buffer[0] = reg;
        std::copy(data.begin(), data.end(), buffer.begin() + 1);

        I2C::Message message = {};
        message.address = m_DeviceAddress;
        message.flags = 0; // Write
        message.length = buffer.size();
        message.data = std::span<uint8_t>(buffer.data(), buffer.size());

        I2C::Options options{};
        options.timeout_ms = 1000;
        options.retries = 2;

        auto error = Transfer(std::span<I2C::Message>(&message, 1), options);
        if (error != I2C::Error::OK)
        {
            throw std::runtime_error("I2C Transfer failed with error code: " + std::to_string(static_cast<int>(error)));
        }
    }

}
