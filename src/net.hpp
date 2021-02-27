#pragma once 

#include <stdint.h>

#include <WinSock2.h>

#include "util.hpp"

const uint16_t MAX_MSG_SIZE = 1024;

enum struct ClientMessageType : uint8_t
{
    JOIN = 0,
    READY,
    START,
    BUZZ,
    PASS_OR_PLAY,
    ANSWER,

    INVALID,
};
enum struct ServerMessageType : uint8_t
{
    JOIN_RESPONSE = 0,
    DESCRIBE_LOBBY,
    START_GAME,
    START_ROUND,
    START_FACEOFF,
    ASK_QUESTION,
    PROMPT_PASS_OR_PLAY,
    PROMPT_FOR_ANSWER,
    PLAYER_BUZZED,
    START_PLAY,
    START_STEAL,
    PLAYER_SAID_SOMETHING,
    DO_A_FLIP,
    DO_AN_EEEEEGGGHHHH,
    END_ROUND,

    INVALID,
};

char *append_byte(char *buf, char val)
{
    *(buf++) = val;
    return buf;
}

char *append_short(char *buf, uint16_t val)
{
    uint16_t n_val = htons(val);
    *(buf++) = n_val;
    *(buf++) = n_val >> 8;
    return buf;
}

char *append_long(char *buf, uint64_t val)
{
    uint64_t n_val = htonll(val);
    *(uint64_t *)buf = n_val;
    buf += sizeof(uint64_t);
    return buf;
}

char *append_string(char *buf, char *str, uint16_t len)
{
    buf = append_short(buf, len);
    memcpy(buf, str, len);
    return buf + len;
}

char *append_string(char *buf, String str)
{
    buf = append_short(buf, str.len);
    memcpy(buf, str.data, str.len);
    return buf + str.len;
}

char *read_byte(char *buf, char *val)
{
    *val = buf[0];
    return buf + 1;
}

char *read_short(char *buf, uint16_t *val)
{
    uint16_t n_val = *(uint16_t *)buf;
    *val = ntohs(n_val);
    return buf + 2;
}

char *read_long(char *buf, uint64_t *val)
{
    uint64_t n_val = *(uint64_t *)buf;
    *val = ntohll(n_val);
    return buf + 8;
}

char *read_string(char *buf, char *output_buf, uint16_t *len)
{
    buf = read_short(buf, len);
    memcpy(output_buf, buf, *len);
    return buf + *len;
}

char *read_string(char *buf, String *output)
{
    buf = read_short(buf, &output->len);
    memcpy(output->data, buf, output->len);
    return buf + output->len;
}

// returns pointer to string within buf, no copying
char *read_string_inplace(char *buf, char **val, uint16_t *len)
{
    buf = read_short(buf, len);
    *val = buf;
    return buf + *len;
}

struct Peer
{
    SOCKET socket;

    char msg_buf[MAX_MSG_SIZE * 2];
    uint32_t received_so_far = 0;

    void pop_message(uint16_t len)
    {
        received_so_far -= len;
        memcpy(msg_buf, &msg_buf[len], received_so_far);
    }

    int recieve_msg(char *dst)
    {
        int received_this_time = recv(socket, msg_buf + received_so_far,
                                      (MAX_MSG_SIZE * 2) - received_so_far, 0);
                                      
        if (received_this_time == 0)
        {
            socket = 0;
            return -1;
        }
        if (received_this_time == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK)
            {
                printf("Failure trying to recieve data : %d\n", err);

                if (err == WSAECONNRESET)
                {
                    socket = 0;
                }
                return -1;
            }
            received_this_time = 0;
        }
        

        received_so_far += received_this_time;

        if (received_so_far >= 2)
        {
            uint16_t expected_len;
            char *data = read_short(msg_buf, &expected_len);

            if (received_so_far >= expected_len)
            {
                memcpy(dst, data, expected_len - 2);
                pop_message(expected_len);
                return expected_len - 2;
            }
        }

        return 0;
    }

    void send_all(char *msg, uint16_t len)
    {
        int sent = 0;
        while (sent < len)
        {
            int sent_this_time;
            if ((sent_this_time = send(socket, msg + sent, len - sent, 0)) == SOCKET_ERROR)
            {
                printf("Failure trying to send data, error : %d, already sent: %d, remaining: %d\n", WSAGetLastError(), sent, len - sent);
                break;
            }
            sent += sent_this_time;
        }
    }
};

