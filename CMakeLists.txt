cmake_minimum_required(VERSION 3.0.2)

# 项目名称
project(hightorque_fdcan)

# 包含头文件目录
include_directories(include)

# 源文件变量
set(SOURCE_FILES
    src/canport.cpp
    src/crc.cpp
    src/fdcan_queue.cpp
)

link_directories(${CMAKE_SOURCE_DIR}/lib)

# 查找 libserialport 和 serial
find_library(SERIALPORT_LIB NAMES serialport serial)

if(NOT SERIALPORT_LIB)
    message(FATAL_ERROR "libserialport not found!")
endif()

find_library(SERIAL_LIB NAMES serial)

if(NOT SERIAL_LIB)
    message(FATAL_ERROR "serial library not found!")
endif()


set(LINK_LIBS ${SERIALPORT_LIB} ${SERIAL_LIB} pthread)


# 添加可执行文件
add_executable(demo1 ${SOURCE_FILES} example/demo1.cpp)
target_link_libraries(demo1 ${LINK_LIBS})

add_executable(demo2 ${SOURCE_FILES} example/demo2.cpp)
target_link_libraries(demo2 ${LINK_LIBS})
