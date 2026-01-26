#include <iostream>
#include "serial_struct.h"
#include <vector>


class canport
{
private:
    cdc_tr_message_data_s cdc_tx_message;
    

    std::vector<std::string> list_serial_ports(const std::string& full_prefix);
    int serial_pid_vid(const char *name);

public:
    canport(uint8_t portx = 1, std::string serial_full_prefix = "/dev/ttyACM");
};

