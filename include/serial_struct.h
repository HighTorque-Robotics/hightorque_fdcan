#ifndef _SERIAL_STRUCT_H
#define _SERIAL_STRUCT_H


#include <stdint.h>


#define PRINT_INFO_G(format, ...) printf("\033[1;32m" format "\033[0m\n", ##__VA_ARGS__)
#define PRINT_INFO(format, ...)   printf(format "\n", ##__VA_ARGS__)
#define PRINT_ERROR(format, ...)  printf("\033[1;31m" format "\033[0m\n", ##__VA_ARGS__)


#define  PROT_DATA_LEN          500
#define  PROT_HEAD              0xF7
#define  PROT_FDCAN_STATE_LEN   3


#define  MODE_NULL              0  // 未定义
#define  MODE_COMM_VERSION      1  // 通信板版本号
#define  MODE_COMM_INIT         2  // 通信板初始化
#define  MODE_FDCAN_PARAM       3  // 自定义 fdcan 的波特率
#define  MODE_CACHE_NUM         4  // 设置缓存的数量

#define  MODE_FDCAN_PASS        10  // fdcan 透传
#define  MODE_MOTOR_STATE       20  // 查询电机状态


#pragma pack(1)

/* fdcan 相关 */
typedef enum __attribute__((packed))
{
    FDCAN_STATUS_UNKNOWN = -1,    // 状态无效
    FDCAN_STATUS_OK,             // 正常
    FDCAN_STATUS_ERROR_WARNING,  // 错误警告--位错误、CRC错误、ACK错误、格式错误（可自行恢复）
    FDCAN_STATUS_ERROR_PASSIVE,  // 被动错误--表现：不在发送，但可接收
    FDCAN_STATUS_BUS_OFF,        // 总线关闭--表现：不在发送或接收任何数据
} __attribute__((packed)) fdcan_fault_t;


typedef struct 
{
    fdcan_fault_t fault;   // 错误码
    uint8_t tx_err_num;    // 发送错误计数
    uint8_t rx_err_num;    // 接收错误计数
} fdcan_state_s;


typedef struct
{
    uint16_t arb_pre;
    uint16_t arb_sjw;
    uint16_t arb_seg1;
    uint16_t arb_seg2;

    uint16_t data_pre;
    uint16_t data_sjw;
    uint16_t data_seg1;
    uint16_t data_seg2;
} fdcan_param_s;


typedef struct 
{
    union 
    {
        struct 
        {
            uint8_t brs_flag : 1;
            uint8_t fdcan_flag : 1;
            uint8_t id_flag : 1;
            uint8_t send_flag: 1;
            uint8_t xxx : 4;  // 占位，暂时无作用
        };
        uint8_t data;
    };
} fdcan_config_s;


typedef struct
{
    uint32_t id;
    fdcan_config_s config;
    uint8_t len;
    uint8_t data[64];
} fdcan_msg_s;



/* head */
typedef struct
{
    uint8_t head;
    uint8_t cmd;
    uint16_t len;
    uint8_t  crc_head;
    uint8_t  crc_data;
} prot_head_field_s;


typedef struct
{
    union
    {
        prot_head_field_s s;
        uint8_t raw[sizeof(prot_head_field_s)];
    };
} prot_head_s;



/* 数据段 */
typedef struct 
{
    union 
    {
        struct 
        {
            uint8_t patch;
            uint8_t minor;
            uint8_t major;
        };
        uint32_t data32;
    };
} version_s;




typedef struct
{
    prot_head_s head;
    union
    {
        struct
        {
            fdcan_state_s fdcan_state;
            union
            {
                fdcan_msg_s fdcan_msg;
                version_s version;
                uint8_t raw[PROT_DATA_LEN - sizeof(prot_head_s) - sizeof(fdcan_state_s)];
            };
        } s;
        uint8_t raw[PROT_DATA_LEN - sizeof(prot_head_s)];
    } data;
} prot_comm2cdc_s;


typedef struct
{
    prot_head_s head;
    union
    {
        fdcan_msg_s fdcan_msg;
        fdcan_param_s fdcan_param;
        uint8_t raw[PROT_DATA_LEN];
    } data;
} prot_cdc2comm_s;

#pragma pack()


#endif