cmake_minimum_required(VERSION 3.16.0)
project(serial)

# Sources
set(serial_SRCS
    src/serial.cc
    include/serial/serial.h
    include/serial/v8stdint.h
)
if(WIN32)
    list(APPEND serial_SRCS src/impl/win.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_win.cc)
else()
    list(APPEND serial_SRCS src/impl/unix.cc)
    list(APPEND serial_SRCS src/impl/list_ports/list_ports_linux.cc)
endif()

# Add serial library
add_library(${PROJECT_NAME} ${serial_SRCS})

# Include headers for both internal and external targets
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} setupapi)
else()
    target_link_libraries(${PROJECT_NAME} rt pthread)
endif()

