#pragma once

#include <stdint.h>

typedef struct _PACKET_HEADER
{
    uint32_t length;
    uint32_t command;
}PACKET_HEADER;

#define COMMAND_LOGIN       0xA0
#define COMMAND_QUERY       0xA2
#define COMMAND_MESSAGE     0xA3

typedef struct _CMD_LOIGN
{
    PACKET_HEADER hdr;
    char username[32];
    char password[32];
}CMD_LOIGN;

typedef struct _CMD_QUERY
{
    PACKET_HEADER hdr;
}CMD_QUERY;

typedef struct _CMD_MESSAGE
{
    PACKET_HEADER hdr;
    char text[64];
}CMD_MESSAGE;

