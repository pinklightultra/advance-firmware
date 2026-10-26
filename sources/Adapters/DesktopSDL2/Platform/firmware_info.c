#include "Adapters/adv/Core/Inc/firmware_info.h"

const firmware_info_t firmware_info = {
    .magic = 0xC0D31355,
    .length = 0,
    .version = 0x72030201, // v3.2.1
    .crc32 = 0,
    .type = 0x00006677,
};
