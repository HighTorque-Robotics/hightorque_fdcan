
#include "serial_struct.h"
#include "serial/serial.h"
#include "fdcan_queue.h"
#include <iostream>
#include <vector>
#include <thread>


typedef enum 
{
    ARB_BAUD_50K,
    ARB_BAUD_100K,
    ARB_BAUD_125K,
    ARB_BAUD_200K,
    ARB_BAUD_250K,
    ARB_BAUD_500K,
    ARB_BAUD_800K,
    ARB_BAUD_1M,
} arb_baud_t;


typedef enum 
{
    DATA_BAUD_100K,
    DATA_BAUD_125K,
    DATA_BAUD_250K,
    DATA_BAUD_500K,
    DATA_BAUD_1M,
    DATA_BAUD_2M,
    DATA_BAUD_4M,
    DATA_BAUD_5M,
} data_baud_t;


class canport
{
public:
    canport(uint8_t portx = 1, uint8_t cache_num = 1, std::string serial_full_prefix = "/dev/ttyACM");
    ~canport();
    void set_cache_num(uint8_t num);

    void fdcan_send();
    uint8_t fdcan_data(uint32_t id, uint8_t *data, uint8_t len, bool fdcan_flag, bool brs_flag, bool ext_flag);

    bool fdcan_queue_pop(uint32_t *id, uint8_t *data, uint8_t *len, fdcan_config_s *config);
    bool fdcan_queue_pop(fdcan_msg_s *fdcan_buf);

    void set_fdcan_baud(arb_baud_t arb_baud, data_baud_t data_baud);
    void set_fdcan_baud(fdcan_param_s *p_fdcan_param);

    fdcan_state_s *get_canport_state(void);
private:
    version_s comm_version;  // 通信板版本号
    version_s sdk_version;   // SDK 版本号
    uint8_t canpot_id = 1;

    serial::Serial ser_dev;
    prot_cdc2comm_s prot_tdata;
    
    std::thread ser_recv_thread;
    fdcan_queue fdcan_queue_rx;

    fdcan_state_s canport_state;

    uint8_t comm_init_flag = 0;
    uint8_t set_fdcan_baud_flag = 0;
    uint8_t set_cache_num_flag = 0;
    
    void serial_init(uint8_t portx, std::string serial_full_prefix);
    void port_tdata_clean(const uint8_t mode, const uint16_t len);
    void comm_init(void);
    void fdcan_init(void);
    uint8_t fdcan_len(bool fdcan_flag, uint8_t size);
    void get_comm_version();
    void fdcan_process(uint8_t *data, uint16_t len);
    void send();
    void recv();
};

