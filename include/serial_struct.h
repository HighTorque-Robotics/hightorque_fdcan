#ifndef _SERIAL_STRUCT_H_
#define _SERIAL_STRUCT_H_


#include <stdint.h>
#include "crc.h"


#define ROS_INFO_G(format, ...) ROS_INFO("\033[1;32m" format "\033[0m", ##__VA_ARGS__)
#define ROS_INFO(format, ...)  printf(format "\n", ##__VA_ARGS__)
#define ROS_ERROR(format, ...) printf("\033[1;31m" format "\033[0m\n", ##__VA_ARGS__)
#define ROS_DEBUG_STREAM(x)


#define  CDC_TR_MESSAGE_DATA_LEN    512


#define  MODE_NULL                  0X00  // 未定义
#define  MODE_RESET_ZERO            0X01  // 重置电机零位
#define  MODE_CONF_WRITE            0X02  // 保存设置
#define  MODE_STOP                  0X03  // 电机停止
#define  MODE_BRAKE                 0X04  // 电机刹车
#define  MODE_SET_NUM               0X05  // 设置通道电机数量，并查询固件版本
#define  MODE_MOTOR_STATE           0X06  // 电机状态
// #define  MODE_CONF_LOAD             0X07  // 还原设置（弃用）
#define  MODE_RESET                 0X08  // 电机重启
// #define  MODE_RUNZERO               0X09  // 上电自动回零
#define  MODE_MOTOR_STATE2          0X0A  // 电机状态2(带模式和错误码)
#define  MODE_MOTOR_VERSION         0X0B  // 电机版本号
#define  MODE_FUN_V                 0X0C  // 设置功能版本号
#define  MODE_BOOTLOADER            0X0D  // 升级通讯板固件
#define  MODE_FDCAN_RESET           0X0E  // 重新初始化 FDCAN
#define  MODE_FDCAN_MOTOR_STATE     0X0F  // 通讯板 FDCAN 报错码 + 电机状态
#define  MODE_TQE_ADJS_FLAG         0X10  // 电机内部力矩修正（电机固件 v4.6.0 开始支持）
#define  MODE_FDCAN_MOTOR_STATE2    0X11  // 通讯板 FDCAN 报错码 + 电机状态2(带模式和错误码)
#define  MODE_FDCAN_TRAN            0X12  // CAN/FDCAN 透传（原始数据收发）


#define COMBINE_VERSION(major, minor, patch) (((major) << 12) | ((minor) << 4) | (patch))
#define GET_MAJOR_VERSION(version) (((version) >> 12) & 0xF)  // 提取 major 版本
#define GET_MINOR_VERSION(version) (((version) >> 4) & 0xFF)  // 提取 minor 版本
#define GET_PATCH_VERSION(version) ((version) & 0xF)          // 提取 patch 版本


#pragma pack(1)
typedef struct
{
    uint8_t major : 4;
    uint8_t minor : 8;
    uint8_t patch : 4;
} version_s, *p_version_s;

typedef struct 
{
    uint8_t head;
    uint8_t cmd;
    uint16_t len;
    uint8_t  crc8;
    uint16_t crc16;
} cdc_tr_message_head_data_s;


typedef struct
{
    union 
    {
        cdc_tr_message_head_data_s s;
        uint8_t data[sizeof(cdc_tr_message_head_data_s)];
    };
} cdc_tr_message_head_s;


typedef struct 
{
    union 
    {
        struct 
        {
            uint8_t brs_flag : 1;    // 比特率切换使能：1-数据段使用高速率，0-全帧同速率
            uint8_t fdcan_flag : 1;  // 帧格式：1-fdcan，0-can
            uint8_t id_flag: 1;      // 标识符类型：1-拓展帧，0-标准帧
            uint8_t : 5;             // 占位，暂时无作用，必须为 0
        };
        uint8_t data;
    };
} cdc_tr_fdcan_tran_config_s;


typedef struct
{
    uint32_t id;
    cdc_tr_fdcan_tran_config_s config;
    uint8_t len;
    uint8_t data[64];
} cdc_tr_fdcan_tran_s;


typedef struct
{
    union
    {
        cdc_rx_motor_version_s motor_version[CDC_TR_MESSAGE_DATA_LEN / sizeof(cdc_rx_motor_version_s)];
        uint8_t data[CDC_TR_MESSAGE_DATA_LEN];
        int16_t data16[CDC_TR_MESSAGE_DATA_LEN / sizeof(int16_t)];
    };
} cdc_tr_message_data_s;


typedef struct 
{
    cdc_tr_message_head_s head;
    cdc_tr_message_data_s data;
} cdc_tr_message_s;

#pragma pack()


#endif