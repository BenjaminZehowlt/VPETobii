#ifndef LIBEYETRACK_VIVE_CONTROL_T_H
#define LIBEYETRACK_VIVE_CONTROL_T_H

#include <cstdint>

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

// Struct should be exactly 64 bytes
PACK(struct vive_control_t {
    uint8_t report_id;
    uint16_t command;
    uint16_t length;
    uint8_t unknown1[5];
    uint8_t unknown2;
    uint8_t unknown3[36];
    uint8_t crc[3];
    uint8_t unknown4[14];
});

#endif //LIBEYETRACK_VIVE_CONTROL_T_H
