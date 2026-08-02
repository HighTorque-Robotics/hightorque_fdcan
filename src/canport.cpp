#include "canport.h"
#include "version.h"
#include "crc.h"

#include <algorithm>



canport::canport(uint8_t portx,  uint8_t cache_num, std::string serial_full_prefix)
{
    canpot_id = portx;
    const auto t1 = std::chrono::steady_clock::now();

    comm_version.data32 = 0;
    sdk_version.data32 = HIGHTORQUE_FDCAN_VERSION;

    PRINT_INFO_G("FDCAN_PASS version: %d.%d.%d", sdk_version.major, sdk_version.minor, sdk_version.patch);

    serial_init(portx, serial_full_prefix);
    get_comm_version();
    comm_init();
    set_cache_num(cache_num);
    if (comm_version.data32 < VER_COMBINE(6, 0, 0))
    {
        PRINT_ERROR("The communication board version is too low!!!");
        exit(1);
    }

    fdcan_init();
    

    const auto t2 = std::chrono::steady_clock::now();
    auto t = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

    std::cout << "初始化耗时：" << t.count() << "ms" << std::endl;
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


uint8_t canport::fdcan_len(bool fdcan_flag, uint8_t size)
{
    uint8_t data_len = 0;

    if (fdcan_flag == 0 && size > 8)
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


void canport::port_tdata_clean(const uint8_t mode, const uint16_t len)
{
    if (prot_tdata.head.s.cmd != mode)
    {
        prot_tdata.head.s.head = PROT_HEAD;
        prot_tdata.head.s.cmd = mode;
        prot_tdata.head.s.len = len;
        memset(&prot_tdata.data, 0, len);
    }
}


void canport::fdcan_init()
{
    port_tdata_clean(MODE_FDCAN_PASS, PROT_DATA_LEN);

    prot_tdata.data.fdcan_msg.data[0];
    prot_tdata.data.fdcan_msg.config.brs_flag = 1;
    prot_tdata.data.fdcan_msg.config.fdcan_flag = 1;
    prot_tdata.data.fdcan_msg.config.id_flag = 1;
    prot_tdata.data.fdcan_msg.config.send_flag = 0;
    
    prot_tdata.head.s.len = sizeof(fdcan_config_s);

    send();
    send();
    prot_tdata.head.s.len = 0;
}


uint8_t canport::fdcan_data(uint32_t id, uint8_t *data, uint8_t len, bool fdcan_flag, bool brs_flag, bool ext_flag)
{
    const uint8_t fdcan_data_len = fdcan_len(fdcan_flag, len);
    const uint8_t fdcan_config_len = sizeof(id) + sizeof(fdcan_config_s) + sizeof(len);

    if (prot_tdata.head.s.len + fdcan_config_len + fdcan_data_len > PROT_DATA_LEN - sizeof(prot_head_s))
    {
        return 1;
    }

    port_tdata_clean(MODE_FDCAN_PASS, 0);

    fdcan_msg_s* const p_fdcan_buf = (fdcan_msg_s *)&prot_tdata.data.raw[prot_tdata.head.s.len];
    prot_tdata.head.s.len += fdcan_data_len + fdcan_config_len;

    p_fdcan_buf->id = id;
    p_fdcan_buf->config.data = 0;
    p_fdcan_buf->config.brs_flag = brs_flag;
    p_fdcan_buf->config.fdcan_flag = fdcan_flag;
    p_fdcan_buf->config.id_flag = ext_flag;
    p_fdcan_buf->config.send_flag = 1;

    p_fdcan_buf->len = fdcan_data_len;
    memcpy(p_fdcan_buf->data, data, fdcan_data_len);

    return 0;
}


bool canport::fdcan_queue_pop(uint32_t *id, uint8_t *data, uint8_t *len, fdcan_config_s *config)
{
    fdcan_msg_s fdcan_buf;
    
    const bool r = fdcan_queue_rx.pop(fdcan_buf);
    if (r == true)
    {
        *id = fdcan_buf.id;
        *len = fdcan_buf.len;
        config->data = fdcan_buf.config.data;
        memcpy(data, fdcan_buf.data, fdcan_buf.len);
    }

    return r;
}


bool canport::fdcan_queue_pop(fdcan_msg_s *fdcan_buf)
{
    return fdcan_queue_rx.pop(*fdcan_buf);
}


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


void canport::get_comm_version()
{
    port_tdata_clean(MODE_COMM_VERSION, 0);

    for (int i = 0; i < 5; i++)
    {
        send();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (comm_version.data32 >= VER_COMBINE(3, 0, 0))
        {
            PRINT_INFO_G("board version: v%d.%d.%d", comm_version.major, comm_version.minor, comm_version.patch);
            return;
        }
    }

    PRINT_ERROR("Failed to retrieve the communication board version number!!!");
    exit(1);
}


void canport::comm_init()
{
    port_tdata_clean(MODE_COMM_INIT, 0);

    prot_tdata.head.s.len = 0;

    comm_init_flag = 0;
    for (int i = 0; i < 5; i++)
    {
        send();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (comm_init_flag != 0)
        {
            PRINT_INFO_G("comm init");
            return;
        }
    }

    PRINT_ERROR("comm init error!");
    exit(1);
}


void canport::set_cache_num(uint8_t num)
{
    port_tdata_clean(MODE_CACHE_NUM, 1);

    prot_tdata.data.raw[0] = num;

    set_cache_num_flag = 0;
    for (int i = 0; i < 5; i++)
    {
        send();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (set_cache_num_flag != 0)
        {
            PRINT_INFO_G("set rdata max num");
            return;
        }
    }

    PRINT_ERROR("set rdata max num");
    exit(1);
}


void canport::set_fdcan_baud(fdcan_param_s *p_fdcan_param)
{
    if (p_fdcan_param == NULL)
    {
        PRINT_ERROR("p_fdcan_param is NULL!");
        exit(1);
    }

    port_tdata_clean(MODE_FDCAN_PARAM, sizeof(fdcan_param_s));

    prot_tdata.data.fdcan_param = *p_fdcan_param;

    set_fdcan_baud_flag = 99;
    for (int i = 0; i < 5; i++)
    {
        send();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if (set_fdcan_baud_flag != 99)
        {
            PRINT_INFO_G("set fdcan baud ok");
            return;
        }
    }

    PRINT_ERROR("set fdcan baud error!");
    exit(1);
}


void canport::set_fdcan_baud(arb_baud_t arb_baud, data_baud_t data_baud)
{
    fdcan_param_s param = {0};

    switch (arb_baud) 
    {
    case ARB_BAUD_50K:
        param.arb_pre = 40;
        param.arb_seg1 = 24;
        param.arb_seg2 = 15;
        param.arb_sjw = 15;
        break;
    case ARB_BAUD_100K:
        param.arb_pre = 40;
        param.arb_seg1 = 16;
        param.arb_seg2 = 3;
        param.arb_sjw = 2;
        break;
    case ARB_BAUD_125K:
        param.arb_pre = 5;
        param.arb_seg1 = 111;
        param.arb_seg2 = 16;
        param.arb_sjw = 15;
        break;
    case ARB_BAUD_200K:
        param.arb_pre = 20;
        param.arb_seg1 = 15;
        param.arb_seg2 = 4;
        param.arb_sjw = 4;
        break;
    case ARB_BAUD_250K:
        param.arb_pre = 20;
        param.arb_seg1 = 12;
        param.arb_seg2 = 3;
        param.arb_sjw = 3;
        break;
    case ARB_BAUD_500K:
        param.arb_pre = 2;
        param.arb_seg1 = 63;
        param.arb_seg2 = 16;
        param.arb_sjw = 16;
        break;
    case ARB_BAUD_800K:
        param.arb_pre = 1;
        param.arb_seg1 = 79;
        param.arb_seg2 = 20;
        param.arb_sjw = 15;
        break;
    case ARB_BAUD_1M:
        param.arb_pre = 1;
        param.arb_seg1 = 59;
        param.arb_seg2 = 20;
        param.arb_sjw = 15;
        break;
    default:
        PRINT_ERROR("fdcan arb buad error!");
        exit(1);
    }

    switch (data_baud) 
    {
    case DATA_BAUD_50K:
        param.data_pre = 40;
        param.data_seg1 = 24;
        param.data_seg2 = 15;
        param.data_sjw = 15;
        break;
    case DATA_BAUD_100K:
        param.data_pre = 40;
        param.data_seg1 = 16;
        param.data_seg2 = 3;
        param.data_sjw = 2;
        break;
    case DATA_BAUD_125K:
        param.data_pre = 5;
        param.data_seg1 = 111;
        param.data_seg2 = 16;
        param.data_sjw = 15;
        break;
    case DATA_BAUD_250K:
        param.data_pre = 20;
        param.data_seg1 = 12;
        param.data_seg2 = 3;
        param.data_sjw = 3;
        break;
    case DATA_BAUD_500K:
        param.data_pre = 2;
        param.data_seg1 = 63;
        param.data_seg2 = 16;
        param.data_sjw = 16;
        break;
    case DATA_BAUD_1M:
        param.data_pre = 1;
        param.data_seg1 = 59;
        param.data_seg2 = 20;
        param.data_sjw = 15;
        break;
    case DATA_BAUD_2M:
        param.data_pre = 1;
        param.data_seg1 = 29;
        param.data_seg2 = 10;
        param.data_sjw = 10;
        break;
    case DATA_BAUD_4M:
        param.data_pre = 1;
        param.data_seg1 = 14;
        param.data_seg2 = 5;
        param.data_sjw = 5;
        break;
    case DATA_BAUD_5M:
        param.data_pre = 2;
        param.data_seg1 = 5;
        param.data_seg2 = 2;
        param.data_sjw = 2;
        break;
    default:
        PRINT_ERROR("fdcan data buad error!");
        exit(1);
    }

    set_fdcan_baud(&param);
}   


fdcan_state_s *canport::get_canport_state()
{
    return &canport_state;
}


void canport::fdcan_send()
{
    send();
    prot_tdata.head.s.len = 0;
}


void canport::fdcan_process(uint8_t *data, uint16_t len)
{
    const uint8_t fdcan_config_len = 5 + sizeof(fdcan_config_s);
    uint32_t index = 0;

    while(len >= fdcan_config_len + index)
    {
        fdcan_msg_s const * p_tran = (fdcan_msg_s *)&data[index];
        fdcan_msg_s fdcan_buf;

        fdcan_buf.config = p_tran->config;
        fdcan_buf.id = p_tran->id;
        fdcan_buf.len = p_tran->len;

        memcpy(fdcan_buf.data, p_tran->data, p_tran->len);
        fdcan_queue_rx.push(fdcan_buf);
        index += p_tran->len + fdcan_config_len;
    }

    
#if 0
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() / 1000.0;
    static double time_old = now_time;

    printf("-----------process len = %d-------------------\n", len);
    printf("time_new - time_old = %fs\r\n", now_time - time_old);
    time_old = now_time;

    fdcan_msg_s fdcan_rx;

    while (fdcan_queue_rx.pop(fdcan_rx))
    {
        printf("id = %X, len = %d\n", fdcan_rx.id, fdcan_rx.len);
        printf("fdcan = %d, brs = %d, id = %d\n", fdcan_rx.config.fdcan_flag, fdcan_rx.config.brs_flag, fdcan_rx.config.id_flag);

        for (int i = 0; i < fdcan_rx.len; i++)
        {
            printf("%2X ", fdcan_rx.data[i]);
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
    prot_comm2cdc_s prot_rdata = {0};

    while(ser_dev.isOpen())
    {
        prot_rdata.head.s.head = 0;
        prot_rdata.head.s.cmd = 0;
        prot_rdata.head.s.len = 0;
        try
        {  
            ser_dev.read(&prot_rdata.head.raw[0], 1);
            if (prot_rdata.head.s.head != PROT_HEAD)
            {
                continue;
            }

            ser_dev.read(&prot_rdata.head.raw[1], 4);
            if (prot_rdata.head.s.crc_head != crc8_ccitt(&prot_rdata.head.raw[1], 3))
            {
                continue;
            }

            ser_dev.read(&prot_rdata.head.raw[5], prot_rdata.head.s.len + 1);
            if (prot_rdata.head.s.crc_data != crc8_ccitt(&prot_rdata.data.raw[0], prot_rdata.head.s.len))
            {
                continue;
            }

            #if 0
            printf("cmd %02X, len = %d   ", prot_rdata.head.s.cmd, prot_rdata.head.s.len);
            for (int i = 0; i < prot_rdata.head.s.len; i++)
            {
                printf("0x%02X ", prot_rdata.data.raw[i]);
            }
            printf("\n");
            #endif

            canport_state = prot_rdata.data.s.fdcan_state;
            if (canport_state.fault > FDCAN_STATUS_ERROR_WARNING || canport_state.fault == FDCAN_STATUS_UNKNOWN)
            {
                PRINT_ERROR("canport[%d] flaut = %d, rx = %d, tx = %d", canpot_id, canport_state.fault, canport_state.rx_err_num, canport_state.tx_err_num);
            }
            else if (canport_state.fault == FDCAN_STATUS_ERROR_WARNING)
            {
                PRINT_INFO("\033[1;32mcanport[%d] flaut = %d, rx = %d, tx = %d\033[0m", canpot_id, canport_state.fault, canport_state.rx_err_num, canport_state.tx_err_num);
            }

            switch (prot_rdata.head.s.cmd)
            {
            case MODE_COMM_VERSION:
                comm_version.major = prot_rdata.data.s.version.major;
                comm_version.minor = prot_rdata.data.s.version.minor;
                comm_version.patch = prot_rdata.data.s.version.patch;
                break;
            case MODE_COMM_INIT:
                comm_init_flag = prot_rdata.data.s.raw[0];
                break;
            case MODE_FDCAN_PARAM:
                set_fdcan_baud_flag = prot_rdata.data.s.raw[0];
                break;
            case MODE_CACHE_NUM:
                set_cache_num_flag = prot_rdata.data.s.raw[0];
                break;
            case MODE_FDCAN_PASS:
                fdcan_process(&prot_rdata.data.raw[3], prot_rdata.head.s.len - 3);
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
