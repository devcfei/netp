#pragma once

#include <stdint.h>

typedef struct _PACKET_HEADER
{
    uint32_t length;
    uint32_t command;
}PACKET_HEADER;

#define COMMAND_MESSAGE 0xA0

typedef struct _CMD_MESSAGE
{
    PACKET_HEADER hdr;
    char text[64];
}CMD_MESSAGE;