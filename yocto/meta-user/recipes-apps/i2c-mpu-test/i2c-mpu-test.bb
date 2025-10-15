LICENSE = "CLOSED"

SRC_URI = "\
    file://CMakeLists.txt \
    file://main.cpp \
    file://MPU6050.cpp \
    file://MPU6050.hpp \
    file://I2CInterface/Src/I2CDevice.cpp \
    file://I2CInterface/Inc/I2CDevice.hpp \
    file://Utilities/Src/FileHandle.cpp \
    file://Utilities/Inc/FileHandle.hpp \
    "

S = "${WORKDIR}"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
do_rm_work[noexec] = "1"

DEPENDS += "fmt"

inherit cmake

do_install() {

    install -d ${D}/home/root
    install -m 0755 ${B}/mpu_6050 ${D}/home/root/

}


FILES:${PN} += "/home/root/mpu_6050"
