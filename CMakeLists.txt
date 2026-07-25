cmake_minimum_required(VERSION 4.0.0)

# 项目名称
project(hightorque_fdcan)

# 包含头文件目录
include_directories(include)

# 先编译 serial 库
add_subdirectory(lib/serial)

# 源文件变量
set(SOURCE_FILES
    src/canport.cpp
    src/crc.cpp
    src/fdcan_queue.cpp
)

find_package(PkgConfig REQUIRED)

include_directories(${SP_INCLUDE_DIRS})
include_directories(lib/serial/include)

# 添加可执行文件
add_executable(demo1 ${SOURCE_FILES} example/demo1.cpp)
target_link_libraries(demo1 serial ${SP_LIBRARIES} pthread)

add_executable(demo2 ${SOURCE_FILES} example/demo2.cpp)
target_link_libraries(demo2 serial ${SP_LIBRARIES} pthread)
