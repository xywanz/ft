// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#ifndef OCG_BSS_PROTOCOL_CRC32C_H_
#define OCG_BSS_PROTOCOL_CRC32C_H_

#include <cstdint>

uint32_t crc32c(uint32_t crc, const uint8_t* data, unsigned int length);

#endif  // OCG_BSS_PROTOCOL_CRC32C_H_
