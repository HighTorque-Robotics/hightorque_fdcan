#include "canport.h"

#include <string>
#include <iostream>
#include <libserialport.h>
#include <dirent.h>
#include <algorithm>


#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


canport::canport(uint8_t portx, std::string serial_full_prefix)
{
    struct sp_port *ser = nullptr;

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
            if (pid == 0xFFFF)
            {
                switch (vid)
                {
                case (0xCAF1):
                case (0xCAE1):
                    r = 1;
                    break;
                default:
                    r = -3;
                    break;
                }
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


