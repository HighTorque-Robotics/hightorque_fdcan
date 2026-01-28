#include "canport.h"

#include <string>
#include <iostream>
#include <libserialport.h>
#include <dirent.h>
#include <algorithm>


canport::canport(uint8_t portx, std::string serial_full_prefix)
{
    board_version.data16 = 0;
    version.data16 = COMBINE_VERSION(0, 0, 0);

    PRINT_INFO_G("FDCAN_PASS version: %d.%d.%d", version.major, version.minor, version.patch);

    serial_init(portx, serial_full_prefix);
    get_board_version();
    if (board_version.data16 < COMBINE_VERSION(4, 8, 8))
    {
        PRINT_ERROR("The communication board version is too low!!!");
        exit(1);
    }

    fdcan_pass_init();
}

canport::~canport()
{
    printf("~canport\n");
    ser_dev.close();

    if (ser_recv_thread.joinable())
    {
        ser_recv_thread.join();
    }
}


uint8_t canport::fdcan_pass_len(uint8_t size)
{
    uint8_t data_len = 0;

    if (fdcan_pass_config_rx.fdcan_flag == 0 && size > 8)
    {
        return 8;
    }

    if (size <= 8)
    {
        data_len = size;
    }
    else if (size <= 12)
    {
        data_len = 12;
    }
    else if (size <= 16)
    {
        data_len = 16;
    }
    else if (size <= 20)
    {
        data_len = 20;
    }
    else if (size <= 24)
    {
        data_len = 24;
    }
    else if (size <= 32)
    {
        data_len = 32;
    }
    else if (size <= 48)
    {
        data_len = 48;
    }
    else 
    {
        data_len = 64;
    }
    
    return data_len;
}


void canport::fdcan_pass_init()
{
    if (cdc_tx_message.head.s.cmd != MODE_FDCAN_PASS)
    {
        cdc_tx_message.head.s.head = 0XF7;
        cdc_tx_message.head.s.cmd = MODE_FDCAN_PASS;
        memset(&cdc_tx_message.data, 0, CDC_TR_MESSAGE_DATA_LEN);
    }

    fdcan_pass_config_rx.data = 0;
    fdcan_pass_config_rx.brs_flag = 1;
    fdcan_pass_config_rx.fdcan_flag = 1;
    fdcan_pass_config_rx.id_flag = 1;
    fdcan_pass_config_rx.send_flag = 0;
    
    cdc_tx_message.head.s.len = sizeof(cdc_tr_fdcan_pass_config_s) + 5;

    send();

    fdcan_pass_config_rx.send_flag = 1;
}


void canport::fdcan_pass_config(bool fdcan_flag, bool brs_flag, bool ext_flag)
{
    fdcan_pass_config_rx.data = 0;
    fdcan_pass_config_rx.fdcan_flag = fdcan_flag;
    fdcan_pass_config_rx.brs_flag = brs_flag;
    fdcan_pass_config_rx.id_flag = ext_flag;
    fdcan_pass_config_rx.send_flag = 1;
}


uint8_t canport::fdcan_pass_data(uint32_t id, uint8_t *data, uint8_t len)
{
    const uint8_t fdcan_data_len = fdcan_pass_len(len);
    const uint8_t fdcan_pass_config_len = sizeof(id) + sizeof(cdc_tr_fdcan_pass_config_s) + sizeof(len);
    if (cdc_tx_message.head.s.len + fdcan_pass_config_len > CDC_TR_MESSAGE_DATA_LEN - sizeof(cdc_tr_message_head_s))
    {
        return 1;
    }

    if (cdc_tx_message.head.s.cmd != MODE_FDCAN_PASS)
    {
        cdc_tx_message.head.s.head = 0XF7;
        cdc_tx_message.head.s.cmd = MODE_FDCAN_PASS;
        cdc_tx_message.head.s.len = 0;
        
        memset(&cdc_tx_message.data, 0, CDC_TR_MESSAGE_DATA_LEN);
    }

    cdc_tr_fdcan_pass_s* const p_fdcan_pass = (cdc_tr_fdcan_pass_s *)&cdc_tx_message.data.data[cdc_tx_message.head.s.len];
    cdc_tx_message.head.s.len += fdcan_data_len + fdcan_pass_config_len;

    p_fdcan_pass->id = id;
    p_fdcan_pass->config = fdcan_pass_config_rx;
    p_fdcan_pass->len = fdcan_data_len;
    memcpy(p_fdcan_pass->data, data, fdcan_data_len);

    return 0;
}


bool canport::fdcan_queue_pop(uint32_t *id, uint8_t *data, uint8_t *len, cdc_tr_fdcan_pass_config_s *config)
{
    cdc_tr_fdcan_pass_s fdcan_pass;
    
    const bool r = fdcan_queue_rx.pop(fdcan_pass);
    if (r == true)
    {
        *id = fdcan_pass.id;
        *len = fdcan_pass.len;
        config->data = fdcan_pass.config.data;
        memcpy(data, fdcan_pass.data, fdcan_pass.len);
    }

    return r;
}


bool canport::fdcan_queue_pop(cdc_tr_fdcan_pass_s *fdcan_pass)
{
    return fdcan_queue_rx.pop(*fdcan_pass);
}


void canport::serial_init(uint8_t portx, std::string serial_full_prefix)
{
    std::vector<std::string> ports = list_serial_ports(serial_full_prefix);
    if (ports.empty())
    {
        PRINT_ERROR("No serial ports found for prefix: %s", serial_full_prefix.c_str());
        exit(1);
    }


    std::vector<std::string> com_board_ports;
    for (const std::string& port : ports) 
    {   
        if (serial_pid_vid(port.c_str()) > 0)
        {
            com_board_ports.push_back(port);
        }
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


int canport::serial_pid_vid(const char *name)
{
    int pid, vid;
    int r = 0;
    struct sp_port *port;
    try
    {
        sp_get_port_by_name(name, &port);
        if (sp_get_port_usb_vid_pid(port, &vid, &pid) != SP_OK) 
        {
            r = -1;
        } 
        else 
        {
            if (pid == 0xFFFF && vid == 0xCAF1)
            {
                r = 1;
            }
            else
            {
                r = -1;
            }
        }

        sp_free_port(port);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        r = -3;
        sp_close(port);
        sp_free_port(port);
    }

    return r;
}


std::vector<std::string> canport::list_serial_ports(const std::string& full_prefix) 
{
    std::string base_path = full_prefix.substr(0, full_prefix.rfind('/') + 1);
    std::string prefix = full_prefix.substr(full_prefix.rfind('/') + 1);
    std::vector<std::string> serial_ports;
    DIR *directory;
    struct dirent *entry;

    directory = opendir(base_path.c_str());
    if (!directory)
    {
        PRINT_ERROR("Could not open the directory %s", base_path.c_str());
        return serial_ports; 
    }

    while ((entry = readdir(directory)) != NULL)
    {
        std::string entryName = entry->d_name;
        if (entryName.find(prefix) == 0)
        { 
            serial_ports.push_back(base_path + entryName);
        }
    }

    closedir(directory);

    std::reverse(serial_ports.begin(), serial_ports.end());

    return serial_ports;
}


void canport::get_board_version()
{
    if (cdc_tx_message.head.s.cmd != MODE_SET_NUM)
    {
        cdc_tx_message.head.s.head = 0XF7;
        cdc_tx_message.head.s.cmd = MODE_SET_NUM;
        cdc_tx_message.head.s.len = 2;
        memset(&cdc_tx_message.data, 0, cdc_tx_message.head.s.len);
    }

    cdc_tx_message.data.data[0] = 10;

    for (int i = 0; i < 50; i++)
    {
        send();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (board_version.data16 >= COMBINE_VERSION(3, 0, 0))
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
    cdc_tx_message.head.s.len = 0;
}


void canport::fdcan_pass_process(uint8_t *data, uint16_t len)
{
    const uint8_t fdcan_pass_config_len = 5 + sizeof(cdc_tr_fdcan_pass_config_s);
    uint32_t index = 0;

    while(len >= fdcan_pass_config_len + index)
    {
        cdc_tr_fdcan_pass_s const * p_tran = (cdc_tr_fdcan_pass_s *)&data[index];
        cdc_tr_fdcan_pass_s fdcan_pass;

        fdcan_pass.config = p_tran->config;
        fdcan_pass.id = p_tran->id;
        fdcan_pass.len = p_tran->len;

        memcpy(fdcan_pass.data, p_tran->data, p_tran->len);
        fdcan_queue_rx.push(fdcan_pass);
        index += p_tran->len + fdcan_pass_config_len;
    }

    
#if 0
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 1000.0;
    static double time_old = 0;

    printf("-----------process len = %d-------------------\n", len);
    printf("time_new - time_old = %fs\r\n", now_time - time_old);
    time_old = now_time;

    cdc_tr_fdcan_pass_s fdcan_pass_rx;

    while (fdcan_queue_rx.pop(fdcan_pass_rx))
    {
        printf("id = %X, len = %d\n", fdcan_pass_rx.id, fdcan_pass_rx.len);
        printf("fdcan = %d, brs = %d, id = %d\n", fdcan_pass_rx.config.fdcan_flag, fdcan_pass_rx.config.brs_flag, fdcan_pass_rx.config.id_flag);

        for (int i = 0; i < fdcan_pass_rx.len; i++)
        {
            printf("%2X ", fdcan_pass_rx.data[i]);
            if (i == 31)
            {
                printf("\n");
            }
        }
        printf("\n\n\n");
    }
#endif
}


void canport::send()
{
    cdc_tx_message.head.s.crc8 = crc8_ccitt(&(cdc_tx_message.head.data[1]), 3);
    cdc_tx_message.head.s.crc16 = crc16_ccitt(&(cdc_tx_message.data.data[0]), cdc_tx_message.head.s.len);

    // uint8_t *byte_ptr = (uint8_t *)&cdc_tx_message.head.s.head;
    // printf("send:\n");
    // for (size_t i = 0; i < cdc_tx_message.head.s.len + 7; i++)
    // {
    //     printf("0x%.2X ", byte_ptr[i]);
    // }
    // printf("\n\n");

    try
    {
        if(ser_dev.isOpen())
        {
            ser_dev.write((const uint8_t *)&cdc_tx_message.head.s.head, cdc_tx_message.head.s.len + sizeof(cdc_tr_message_head_s));
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
    uint8_t crc8 = 0;
    uint16_t crc16 = 0;
    cdc_tr_message_data_s cdc_rx_data = {0};

    while(ser_dev.isOpen())
    {
        cdc_tr_message_head_data_s cdc_rx_head = {0};
        try
        {  
            ser_dev.read(&(cdc_rx_head.head), 1);
            if (cdc_rx_head.head != 0xF7)
            {
                continue;
            }

            ser_dev.read(&(cdc_rx_head.cmd), 4);
            if (cdc_rx_head.crc8 != crc8_ccitt((uint8_t *)&(cdc_rx_head.cmd), 3))
            {
                continue;
            }

            ser_dev.read((uint8_t *)&crc16, 2);
            ser_dev.read((uint8_t *)&cdc_rx_data, cdc_rx_head.len);

            if (crc16 != crc16_ccitt((const uint8_t *)&cdc_rx_data.data, cdc_rx_head.len))
            {
                continue;
            }

            // printf("cmd %02X  ", cdc_rx_head.cmd);
            // for (int i = 0; i < cdc_rx_head.len; i++)
            // {
            //     printf("0x%02X ", cdc_rx_data.data[i]);
            // }
            // printf("\n");

            switch (cdc_rx_head.cmd)
            {
            case MODE_SET_NUM:
                {
                    board_version.major = cdc_rx_data.data[2];
                    board_version.minor = cdc_rx_data.data[3];
                    board_version.patch = cdc_rx_head.len == 4 ? 0 : cdc_rx_data.data[4];
                }
                break;
            case MODE_FDCAN_PASS:
                {
                    fdcan_pass_process(cdc_rx_data.data, cdc_rx_head.len);
                }
                break;
            default:
                break;
            }
        }
        catch(const std::exception& e)
        {
            // std::cerr << "\033[1;31m" << e.what() << "\033[0m" << '\n';
            ser_dev.close();
        }
    }
}
