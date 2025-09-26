
# AXI IIC IP Example 

## Setup Environment
- Vivado 2024.2
- Petalinux 2024.2
- MPU 6050 Sensor
- PYNQ Z1 Board
- Ubuntu 24.04


## Build Vivado Project
```bash
source <vivado_shell_script>
cd vivado
make
```

## Build Petalinux Project
```bash
petalinux-create --type project --template zynq --name <project_name> 
petalinux-config --get-hw-description ../vivado/pynq_z1.xsa --silentconfig
```
Note: Replace meta-user folder in the repository with the meta-user folder in project.
```bash
petalinux-build
petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --u-boot images/linux/u-boot.elf --fpga images/linux/system.bit --force
petalinux-package wic
gedit build/rootfs.wks 
```
Note: default wks image's partitions are high(2G, 4G), you can change to 100M, 300M respectively
```bash
petalinux-package wic --wks build/rootfs.wks
```