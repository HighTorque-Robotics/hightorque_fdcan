#ifndef _VERSION_H
#define _VERSION_H


// #include "serial_struct.h"



#define VER_COMBINE(major, minor, patch)    ((uint32_t)(((major) << 16) | ((minor) << 8) | (patch)))
#define VER_GET_MAJOR(version)              ((uint8_t)(((version) >> 16) & 0xFF))
#define VER_GET_MINOR(version)              ((uint8_t)(((version) >> 8) & 0xFF))
#define VER_GET_PATCH(version)              ((uint8_t)(((version) >> 0) & 0xFF))


#define  HIGHTORQUE_FDCAN_VERSION  VER_COMBINE(1, 0, 0)  // 版本号


#endif

