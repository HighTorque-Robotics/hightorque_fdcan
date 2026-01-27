#include <iostream>
#include "serial_struct.h"
#include <vector>
#include "serial/serial.h"
#include <thread>


class canport
{
public:
    canport(uint8_t portx = 1, std::string serial_full_prefix = "/dev/ttyACM");
    ~canport();

    void send(cdc_tr_message_s *cdc_tx_message);

private:
    serial::Serial ser_dev;
    cdc_tr_message_s cdc_tx_message;
    version_s board_version;
    std::thread ser_recv_thread;
    
    void serial_init(uint8_t portx, std::string serial_full_prefix);
    std::vector<std::string> list_serial_ports(const std::string& full_prefix);
    int serial_pid_vid(const char *name);
    void get_board_version();
    void recv();
};

