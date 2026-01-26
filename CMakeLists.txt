cmake_minimum_required(VERSION 3.10)

# 项目名称
project(HighTorqueFDCANTran)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 包含头文件目录
include_directories(include)

# 源文件变量
set(SOURCE_FILES
    src/canport.cpp
    src/crc.cpp
)

# 查找 libserialport
find_library(SERIALPORT_LIB NAMES serialport)

if(NOT SERIALPORT_LIB)
    message(FATAL_ERROR "libserialport not found!")
endif()

# 添加可执行文件
add_executable(demo ${SOURCE_FILES} example/demo.cpp)
target_link_libraries(demo ${SERIALPORT_LIB})

add_executable(demo1 ${SOURCE_FILES} example/demo1.cpp)
target_link_libraries(demo1 ${SERIALPORT_LIB})
