#include "canport.h"
#include "version.h"
#include "crc.h"

#include <string>
#include <iostream>
#include <dirent.h>
#include <algorithm>


canport::canport(uint8_t portx, std::string serial_full_prefix)
{
    board_version.data32 = 0;
    version.data32 = HIGHTORQUE_FDCAN_VERSION;

    PRINT_INFO_G("FDCAN_PASS version: %d.%d.%d", version.major, version.minor, version.patch);

    serial_init(portx, serial_full_prefix);
    get_board_version();
    if (board_version.data32 < VER_COMBINE(4, 8, 8))
    {
        PRINT_ERROR("The communication board version is too low!!!");
        exit(1);
    }

    // fdcan_init();
}

canport::~canport()
{
    printf("canport close\n");
    ser_dev.close();

    if (ser_recv_thread.joinable())
    {
        ser_recv_thread.join();
    }
}


// uint8_t canport::fdcan_len(uint8_t size)
// {
//     uint8_t data_len = 0;

//     if (fdcan_config_rx.fdcan_flag == 0 && size > 8)
//     {
//         return 8;
//     }

//     if (size <= 8)
//     {
//         data_len = size;
//     }
//     else if (size <= 12)
//     {
//         data_len = 12;
//     }
//     else if (size <= 16)
//     {
//         data_len = 16;
//     }
//     else if (size <= 20)
//     {
//         data_len = 20;
//     }
//     else if (size <= 24)
//     {
//         data_len = 24;
//     }
//     else if (size <= 32)
//     {
//         data_len = 32;
//     }
//     else if (size <= 48)
//     {
//         data_len = 48;
//     }
//     else 
//     {
//         data_len = 64;
//     }
    
//     return data_len;
// }


// void canport::fdcan_init()
// {
//     if (cdc_tx_message.head.s.cmd != MODE_FDCAN_PASS)
//     {
//         cdc_tx_message.head.s.head = 0XF7;
//         cdc_tx_message.head.s.cmd = MODE_FDCAN_PASS;
//         memset(&cdc_tx_message.data, 0, CDC_TR_MESSAGE_DATA_LEN);
//     }

//     fdcan_config_rx.data = 0;
//     fdcan_config_rx.brs_flag = 1;
//     fdcan_config_rx.fdcan_flag = 1;
//     fdcan_config_rx.id_flag = 1;
//     fdcan_config_rx.send_flag = 0;
    
//     cdc_tx_message.head.s.len = sizeof(cdc_tr_fdcan_config_s) + 5;

//     send();

//     fdcan_config_rx.send_flag = 1;
//     cdc_tx_message.head.s.len = 0;
// }


// void canport::fdcan_config(bool fdcan_flag, bool brs_flag, bool ext_flag)
// {
//     fdcan_config_rx.data = 0;
//     fdcan_config_rx.fdcan_flag = fdcan_flag;
//     fdcan_config_rx.brs_flag = brs_flag;
//     fdcan_config_rx.id_flag = ext_flag;
//     fdcan_config_rx.send_flag = 1;
// }


// uint8_t canport::fdcan_data(uint32_t id, uint8_t *data, uint8_t len)
// {
//     const uint8_t fdcan_data_len = fdcan_len(len);
//     const uint8_t fdcan_config_len = sizeof(id) + sizeof(cdc_tr_fdcan_config_s) + sizeof(len);

//     if (cdc_tx_message.head.s.len + fdcan_config_len + fdcan_data_len > CDC_TR_MESSAGE_DATA_LEN - sizeof(cdc_tr_message_head_s))
//     {
//         return 1;
//     }

//     if (cdc_tx_message.head.s.cmd != MODE_FDCAN_PASS)
//     {
//         cdc_tx_message.head.s.head = 0XF7;
//         cdc_tx_message.head.s.cmd = MODE_FDCAN_PASS;
//         cdc_tx_message.head.s.len = 0;
        
//         memset(&cdc_tx_message.data, 0, CDC_TR_MESSAGE_DATA_LEN);
//     }

//     cdc_tr_fdcan_s* const p_fdcan_buf = (cdc_tr_fdcan_s *)&cdc_tx_message.data.data[cdc_tx_message.head.s.len];
//     cdc_tx_message.head.s.len += fdcan_data_len + fdcan_config_len;

//     p_fdcan_buf->id = id;
//     p_fdcan_buf->config = fdcan_config_rx;
//     p_fdcan_buf->len = fdcan_data_len;
//     memcpy(p_fdcan_buf->data, data, fdcan_data_len);

//     return 0;
// }


// uint8_t canport::fdcan_data(uint32_t id, uint8_t *data, uint8_t len, bool fdcan_flag, bool brs_flag, bool ext_flag)
// {
//     const uint8_t fdcan_data_len = fdcan_len(len);
//     const uint8_t fdcan_config_len = sizeof(id) + sizeof(cdc_tr_fdcan_config_s) + sizeof(len);

//     if (cdc_tx_message.head.s.len + fdcan_config_len + fdcan_data_len > CDC_TR_MESSAGE_DATA_LEN - sizeof(cdc_tr_message_head_s))
//     {
//         return 1;
//     }

//     if (cdc_tx_message.head.s.cmd != MODE_FDCAN_PASS)
//     {
//         cdc_tx_message.head.s.head = 0XF7;
//         cdc_tx_message.head.s.cmd = MODE_FDCAN_PASS;
//         cdc_tx_message.head.s.len = 0;
        
//         memset(&cdc_tx_message.data, 0, CDC_TR_MESSAGE_DATA_LEN);
//     }

//     cdc_tr_fdcan_s* const p_fdcan_buf = (cdc_tr_fdcan_s *)&cdc_tx_message.data.data[cdc_tx_message.head.s.len];
//     cdc_tx_message.head.s.len += fdcan_data_len + fdcan_config_len;

//     p_fdcan_buf->id = id;
//     p_fdcan_buf->config.data = 0;
//     p_fdcan_buf->config.brs_flag = brs_flag;
//     p_fdcan_buf->config.fdcan_flag = fdcan_flag;
//     p_fdcan_buf->config.id_flag = ext_flag;
//     p_fdcan_buf->config.send_flag = 1;

//     p_fdcan_buf->len = fdcan_data_len;
//     memcpy(p_fdcan_buf->data, data, fdcan_data_len);

//     return 0;
// }


// bool canport::fdcan_queue_pop(uint32_t *id, uint8_t *data, uint8_t *len, cdc_tr_fdcan_config_s *config)
// {
//     cdc_tr_fdcan_s fdcan_buf;
    
//     const bool r = fdcan_queue_rx.pop(fdcan_buf);
//     if (r == true)
//     {
//         *id = fdcan_buf.id;
//         *len = fdcan_buf.len;
//         config->data = fdcan_buf.config.data;
//         memcpy(data, fdcan_buf.data, fdcan_buf.len);
//     }

//     return r;
// }


// bool canport::fdcan_queue_pop(cdc_tr_fdcan_s *fdcan_buf)
// {
//     return fdcan_queue_rx.pop(*fdcan_buf);
// }


void canport::serial_init(uint8_t portx, std::string serial_full_prefix)
{
    std::vector<serial::PortInfo> all_ports = serial::list_ports();
    std::vector<std::string> com_board_ports;

    for (const auto& port_info : all_ports)
    {
        if (port_info.port.find(serial_full_prefix) == std::string::npos)
        {
            continue;
        }

        std::string hw = port_info.hardware_id;
        auto pos = hw.find("VID:PID=");
        if (pos == std::string::npos)
        {
            continue;
        }

        std::string vid_pid = hw.substr(pos + 8);
        auto colon = vid_pid.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }

        std::string vid = vid_pid.substr(0, colon);
        std::string pid_part = vid_pid.substr(colon + 1);
        auto space = pid_part.find(' ');
        if (space != std::string::npos)
        {
            pid_part = pid_part.substr(0, space);
        }
        std::string pid = pid_part;

        std::transform(vid.begin(), vid.end(), vid.begin(), ::toupper);
        std::transform(pid.begin(), pid.end(), pid.begin(), ::toupper);
        if (vid == "CAF1" && pid == "FFFF")
        {
            
            com_board_ports.push_back(port_info.port);
        }
    }

    if (com_board_ports.empty())
    {
        PRINT_ERROR("No serial ports found for prefix: %s", serial_full_prefix.c_str());
        exit(1);
    }

    PRINT_INFO("Detected serial ports: ");
    for (const auto& port_name : com_board_ports)
    {
        PRINT_INFO("%s", port_name.c_str());
    }

    if (portx > 0 && com_board_ports.size() < portx)
    {
        PRINT_ERROR("Requested port index %d is invalid. Only %ld communication ports available (index starts from 1).", portx, com_board_ports.size());
        exit(1);
    }

    const std::string port_name = com_board_ports[portx - 1].c_str();
    PRINT_INFO_G("port_name: %s", port_name.c_str());

    ser_dev.setPort(port_name.c_str());
    ser_dev.setBaudrate(4000000);
    serial::Timeout timeout = serial::Timeout::simpleTimeout(100);
    ser_dev.setTimeout(timeout);

    try
    {
        ser_dev.open();
    }
    catch(const std::exception& e)
    {
        PRINT_ERROR("serial open err!!!");
        exit(1);
    }
    
    PRINT_INFO_G("Serial Port %s initialized", port_name.c_str());
    ser_recv_thread = std::thread(&canport::recv, this);
}


void canport::get_board_version()
{
    if (prot_tdata.head.s.cmd != MODE_COMM_VERSION)
    {
        prot_tdata.head.s.head = PROT_HEAD;
        prot_tdata.head.s.cmd = MODE_COMM_VERSION;
        prot_tdata.head.s.len = 1;
        memset(&prot_tdata.data, 0, prot_tdata.head.s.len);
    }

    for (int i = 0; i < 50; i++)
    {
        send();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (board_version.data32 >= VER_COMBINE(3, 0, 0))
        {
            PRINT_INFO_G("board version: %d.%d.%d\n", board_version.major, board_version.minor, board_version.patch);
            return;
        }
    }

    PRINT_ERROR("Failed to retrieve the communication board version number!!!");
}


void canport::fdcan_send()
{
    send();
    // cdc_tx_message.head.s.len = 0;
}


// void canport::fdcan_process(uint8_t *data, uint16_t len)
// {
//     const uint8_t fdcan_config_len = 5 + sizeof(cdc_tr_fdcan_config_s);
//     uint32_t index = 0;

//     while(len >= fdcan_config_len + index)
//     {
//         cdc_tr_fdcan_s const * p_tran = (cdc_tr_fdcan_s *)&data[index];
//         cdc_tr_fdcan_s fdcan_buf;

//         fdcan_buf.config = p_tran->config;
//         fdcan_buf.id = p_tran->id;
//         fdcan_buf.len = p_tran->len;

//         memcpy(fdcan_buf.data, p_tran->data, p_tran->len);
//         fdcan_queue_rx.push(fdcan_buf);
//         index += p_tran->len + fdcan_config_len;
//     }

    
// #if 0
//     const auto now = std::chrono::system_clock::now();
//     const auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 1000.0;
//     static double time_old = 0;

//     printf("-----------process len = %d-------------------\n", len);
//     printf("time_new - time_old = %fs\r\n", now_time - time_old);
//     time_old = now_time;

//     cdc_tr_fdcan_s fdcan_rx;

//     while (fdcan_queue_rx.pop(fdcan_rx))
//     {
//         printf("id = %X, len = %d\n", fdcan_rx.id, fdcan_rx.len);
//         printf("fdcan = %d, brs = %d, id = %d\n", fdcan_rx.config.fdcan_flag, fdcan_rx.config.brs_flag, fdcan_rx.config.id_flag);

//         for (int i = 0; i < fdcan_rx.len; i++)
//         {
//             printf("%2X ", fdcan_rx.data[i]);
//             if (i == 31)
//             {
//                 printf("\n");
//             }
//         }
//         printf("\n\n\n");
//     }
// #endif
// }


void canport::send()
{
    prot_tdata.head.s.crc_head = crc8_ccitt(&(prot_tdata.head.raw[1]), 3);
    prot_tdata.head.s.crc_data = crc8_ccitt(&(prot_tdata.data.raw[0]), prot_tdata.head.s.len);

    #if 0
    uint8_t *byte_ptr = (uint8_t *)&prot_tdata.head.s.head;
    printf("send:\n");
    for (size_t i = 0; i < prot_tdata.head.s.len + 6; i++)
    {
        printf("0x%.2X ", byte_ptr[i]);
    }
    printf("\n\n");
    #endif

    try
    {
        if(ser_dev.isOpen())
        {
            ser_dev.write((const uint8_t *)&prot_tdata.head.s.head, prot_tdata.head.s.len + sizeof(prot_head_s));
        }
    }
    catch(const std::exception& e)
    {
        PRINT_ERROR("serial send err: %s", e.what());
        ser_dev.close();
    }
}


void canport::recv()
{

}
