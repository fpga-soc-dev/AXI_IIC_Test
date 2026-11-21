
# AXI IIC IP Core Example — PYNQ-Z1

A minimal, end-to-end example of using the **Xilinx AXI IIC IP core** on a
PYNQ-Z1 (Zynq-7020) board to talk to an **MPU-6050** IMU over I²C from Linux.

The project covers the full stack:

1. **Hardware (Vivado)** — block design with `axi_iic` connected to the PS,
   plus tri-state I/O buffers in the VHDL top level.
2. **Linux (Yocto)** — a Scarthgap image whose device tree is generated from the
   exported `.xsa`, exposing the core as `/dev/i2c-0`.
3. **Application (C++20)** — a userspace driver that uses the standard Linux
   `I2C_RDWR` ioctl to read/write the MPU-6050 registers.

---

## Overview

| Item        | Value                                            |
|-------------|--------------------------------------------------|
| Board       | PYNQ-Z1 (Zynq-7020, `xc7z020clg400-1`)           |
| Sensor      | MPU-6050 (6-axis IMU), I²C address `0x68`        |
| IP core     | `xilinx.com:ip:axi_iic` @ **0x4160_0000**, 400 kHz |
| I²C pins    | `SCL` → **P16**, `SDA` → **P15** (3.3 V LVCMOS33) |
| Linux device| `/dev/i2c-0` (Xilinx I²C driver, `i2c-xiic`)     |
| Toolchain   | Vivado **2024.2**, Yocto **Scarthgap** (kas)     |

## Setup environment

- Vivado 2024.2
- Yocto Scarthgap + [kas](https://kas.readthedocs.io/)
- MPU-6050 sensor module
- PYNQ-Z1 board
- Ubuntu 22.04 (or a similar Linux host)

## Hardware design

### Block design (`Vivado/scripts/pynq_z1_bd.tcl`)

The block design contains:

- `processing_system7` — the Zynq ARM Cortex-A9 PS.
- `axi_iic` — the AXI I²C controller:
  - `CONFIG.IIC_FREQ_KHZ = 400` (Fast mode).
  - Base address `0x4160_0000`.
  - Interrupt `iic2intc_irpt` wired back to the PS.
- `proc_sys_reset`, `xlconcat`, `smartconnect` — reset / interrupt / AXI glue.

### Tri-state I/O in the VHDL top level

The AXI IIC exposes separate **data-in / data-out / tristate** ports
(`sda_i`, `sda_o`, `sda_t`, `scl_i`, `scl_o`, `scl_t`). The VHDL top level
(`Vivado/src/pynq_z1_top.vhd`) uses `IOBUF` primitives to combine each pair
into a single bidirectional pin:

```vhdl
IIC_AXI_scl_iobuf : component IOBUF
  port map (
    I  => IIC_scl_o,
    IO => AXI_I2C_SCL,
    O  => IIC_scl_i,
    T  => IIC_scl_t
  );

IIC_AXI_sda_iobuf : component IOBUF
  port map (
    I  => IIC_sda_o,
    IO => AXI_I2C_SDA,
    O  => IIC_sda_i,
    T  => IIC_sda_t
  );
```

This tri-state handling is the key step when routing the AXI IIC to physical
pins.

### Pin constraints (`Vivado/src/pynq_z1.xdc`)

```tcl
set_property -dict {PACKAGE_PIN P16 IOSTANDARD LVCMOS33} [get_ports AXI_I2C_SCL]
set_property -dict {PACKAGE_PIN P15 IOSTANDARD LVCMOS33} [get_ports AXI_I2C_SDA]
```

### MPU-6050 wiring

| MPU-6050 | PYNQ-Z1 |
|----------|---------|
| VCC      | 3V3     |
| GND      | GND     |
| SCL      | P16     |
| SDA      | P15     |

> The I²C lines are 3.3 V LVCMOS33. If your Pmod wiring does not include
> pull-up resistors, add 4.7 kΩ pull-ups to 3V3 on both SCL and SDA.

## Linux / device tree

The `axi_iic` node is **not written by hand**. During the Yocto build,
`meta-xilinx-tools` generates the device tree (`system-conf.dtsi`) from the
exported `pynq_z1_top.xsa`, which maps the core to `/dev/i2c-0` (driver
`i2c-xiic`). `yocto/meta-user/recipes-bsp/device-tree/files/system-user.dtsi`
only adds board-level pieces (console bootargs, Ethernet PHY, USB, SPI flash).

## Application (`app/i2c-mpu-test`)

A C++20 application that uses the standard Linux I²C interface:

- `Utilities/FileHandle` — RAII wrapper around `open()` / `close()`.
- `I2CInterface/I2CDevice` — opens `/dev/i2c-0`, performs transactions with
  `ioctl(fd, I2C_RDWR, ...)`, and provides `ReadSingle`, `WriteSingle`,
  `ReadBurst`, `WriteBurst`.
- `MPU6050` — register map, configuration (clock source, gyro/accel scale,
  sample rate, FIFO), and reading of accelerometer / gyro / temperature.
- `main.cpp` — resets the sensor, verifies the connection, then prints values
  every second.

The device path and address are set in `main.cpp`:

```cpp
const std::string devicePath = "/dev/i2c-0";
const uint8_t deviceAddress = 0x68;
```

## Build

### 1. Hardware — Vivado

```bash
source <Vivado_install>/settings64.sh
cd Vivado
make
```

`make` runs the Tcl scripts to create the project, build the bitstream, and
export `pynq_z1_top.xsa` (plus `pynq_z1_top.bit`) at the `Vivado/` root.

### 2. Linux image — Yocto (kas)

Prepare the host for Yocto:

- [Preparing the build host](https://docs.yoctoproject.org/singleindex.html#preparing-the-build-host)
- [System requirements (Scarthgap)](https://docs.yoctoproject.org/scarthgap/ref-manual/system-requirements.html#system-requirements)

Install `kas`:

```bash
pip install kas
```

Build:

```bash
cd yocto
make
```

`make` copies `../Vivado/pynq_z1_top.xsa` into the HDF recipe and runs
`kas build build.yaml`. The resulting SD-card image is the `.wic` file under
`yocto/build/tmp-glibc/deploy/images/pynq-z1/`
(e.g. `image-pynq-z1-pynq-z1.rootfs.wic`).

### 3. Flash & run

1. Flash the `.wic` image to a microSD card (e.g. with Balena Etcher).
2. Insert the card, connect the USB UART (115200 8N1), and power the board.
3. Log in as `root` and run:

```bash
/home/root/mpu_6050
```

Expected output:

```
MPU6050 reset successfully.
MPU6050 connection successful.
<accelerometer / gyro / temperature values, once per second>
```

## Repository layout

```
.
├── Vivado/                  # Hardware: block design, VHDL top, constraints
│   ├── scripts/             #   create_project.tcl, pynq_z1_bd.tcl, build_bitstream.tcl
│   └── src/                 #   pynq_z1_top.vhd, pynq_z1.xdc
├── yocto/                   # Linux image: build.yaml + meta-user layer
│   └── meta-user/
│       ├── recipes-bsp/     #   device-tree, HDF, u-boot
│       ├── recipes-kernel/  #   kernel config
│       └── recipes-apps/    #   i2c-mpu-test recipe
└── app/
    └── i2c-mpu-test/        # C++20 MPU-6050 test application
```

## References

- [AXI IIC Bus Interface — Product Guide (PG090)](https://docs.amd.com/r/en-US/pg090-axi-iic)
- [PYNQ-Z1 Reference Manual](https://digilent.com/reference/programmable-logic/pynq-z1/reference-manual)
- [MPU-6050 Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
