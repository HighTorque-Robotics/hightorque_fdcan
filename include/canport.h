
#include "serial_struct.h"
#include "serial/serial.h"
#include "fdcan_queue.h"
#include <iostream>
#include <vector>
#include <thread>


class canport
{
public:
    canport(uint8_t portx = 1, std::string serial_full_prefix = "/dev/ttyACM");
    ~canport();

    void fdcan_send();
    void fdcan_config(bool fdcan_flag, bool brs_flag, bool ext_flag);
    uint8_t fdcan_data(uint32_t id, uint8_t *data, uint8_t len);
    uint8_t fdcan_data(uint32_t id, uint8_t *data, uint8_t len, bool fdcan_flag, bool brs_flag, bool ext_flag);

    bool fdcan_queue_pop(uint32_t *id, uint8_t *data, uint8_t *len, cdc_tr_fdcan_config_s *config);
    bool fdcan_queue_pop(cdc_tr_fdcan_s *fdcan_buf);
    

private:
    version_s board_version;
    version_s version;

    serial::Serial ser_dev;
    cdc_tr_message_s cdc_tx_message;
    
    std::thread ser_recv_thread;
    cdc_tr_fdcan_config_s fdcan_config_rx;
    fdcan_queue fdcan_queue_rx;
    
    void serial_init(uint8_t portx, std::string serial_full_prefix);
    std::vector<std::string> list_serial_ports(const std::string& full_prefix);
    void fdcan_init(void);
    uint8_t fdcan_len(uint8_t size);
    int serial_pid_vid(const char *name);
    void get_board_version();
    void fdcan_process(uint8_t *data, uint16_t len);
    void send();
    void recv();
};

