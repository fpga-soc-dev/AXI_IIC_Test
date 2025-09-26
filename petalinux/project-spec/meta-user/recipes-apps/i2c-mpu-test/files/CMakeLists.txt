cmake_minimum_required(VERSION 3.20)

project(mpu_6050 
	VERSION 1.0.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)

set(SOURCES
    Utilities/Src/FileHandle.cpp
    I2CInterface/Src/I2CDevice.cpp
    MPU6050.cpp
    main.cpp
)


# Create the executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        Utilities/Inc/
        I2CInterface/Inc/
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        fmt::fmt
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)


