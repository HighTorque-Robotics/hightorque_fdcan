#include "canport.h"

#include <string>
#include <iostream>
#include <libserialport.h>
#include <dirent.h>
#include <algorithm>


canport::canport(uint8_t portx, std::string serial_full_prefix)
{
    board_version.data16 = 0;

    serial_init(portx, serial_full_prefix);
    get_board_version();

}

canport::~canport()
{
    ser_dev.close();

    if (ser_recv_thread.joinable())
    {
        ser_recv_thread.join();
    }
}


void canport::serial_init(uint8_t portx, std::string serial_full_prefix)
{
    std::vector<std::string> ports = list_serial_ports(serial_full_prefix);
    if (ports.empty())
    {
        throw std::runtime_error("No serial ports found for prefix: " + serial_full_prefix);
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
        throw std::runtime_error(
            "Requested port index " + std::to_string(portx) +
            " is invalid. Only " + std::to_string(com_board_ports.size()) +
            " communication ports available (index starts from 1).");
    }

    const std::string port_name = com_board_ports[portx - 1].c_str();
    std::cout << "port_name: " << port_name << std::endl;

    ser_dev.setPort(port_name.c_str());
    ser_dev.setBaudrate(4000000);
    serial::Timeout timeout = serial::Timeout::simpleTimeout(1000);
    ser_dev.setTimeout(timeout);

    try
    {
        ser_dev.open();
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("serial open err!!!");
    }
    
    if (ser_dev.isOpen())
    {
        std::cout << "Serial Port " << port_name << " initialized" << std::endl;
    }

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

        // 关闭端口
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
        std::cerr << "\033[1;31m << Could not open the directory " << base_path << "\033[0m" << std::endl;
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
        send(&cdc_tx_message);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (board_version.data16 >= COMBINE_VERSION(3, 0, 0))
        {
            printf("board version: %d.%d.%d\n", board_version.major, board_version.minor, board_version.patch);
            return;
        }
    }

    throw std::runtime_error("Failed to retrieve the communication board version number!!!");
}


void canport::send(cdc_tr_message_s *cdc_tx_message)
{
    cdc_tx_message->head.s.crc8 = crc8_ccitt(&(cdc_tx_message->head.data[1]), 3, 0xFF);
    cdc_tx_message->head.s.crc16 = crc16_ccitt(0xFFFF, &(cdc_tx_message->data.data[0]), cdc_tx_message->head.s.len);

    // uint8_t *byte_ptr = (uint8_t *)&cdc_tr_message->head.s.head;
    // printf("send:\n");
    // for (size_t i = 0; i < cdc_tr_message->head.s.len + 7; i++)
    // {
    //     printf("0x%.2X ", byte_ptr[i]);
    // }
    // printf("\n\n");

    try
    {
        if(ser_dev.isOpen())
        {
            ser_dev.write((const uint8_t *)&cdc_tx_message->head.s.head, cdc_tx_message->head.s.len + sizeof(cdc_tr_message_head_s));
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
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
            if (cdc_rx_head.crc8 != crc8_ccitt((uint8_t *)&(cdc_rx_head.cmd), 3, 0xFF))
            {
                continue;
            }

            ser_dev.read((uint8_t *)&crc16, 2);
            ser_dev.read((uint8_t *)&cdc_rx_data, cdc_rx_head.len);

            if (crc16 != crc16_ccitt(0xFFFF, (const uint8_t *)&cdc_rx_data.data, cdc_rx_head.len))
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
            
            default:
                break;
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << "\033[1;31m" << e.what() << "\033[0m" << '\n';
            ser_dev.close();
        }
    }
}
