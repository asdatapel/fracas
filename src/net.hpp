#pragma once

#include <stdint.h>

#include <WinSock2.h>

#include "common.hpp"
#include "util.hpp"

const uint16_t MAX_MSG_SIZE = 1024;

enum struct ClientMessageType : uint8_t
{
    LIST_GAMES,
    CREATE_GAME,
    JOIN_GAME,
    LEAVE_GAME,

    HOST_START_GAME,
    HOST_START_ASK_QUESTION,
    HOST_RESPOND_TO_ANSWER,
    HOST_END_GAME,

    READY,
    BUZZ,
    PASS_OR_PLAY,
    ANSWER,

    CELEBRATE,

    INVALID,
};

enum struct ServerMessageType : uint8_t
{
    LIST_GAMES_RESPONSE = 0,
    JOIN_GAME_RESPONSE,

    JOIN_RESPONSE,
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

    GAME_CREATED,
    GAME_ENDED,

    PLAYER_LEFT,

    INVALID,
};

struct GameProperties
{
    GameId owner;

    AllocatedString<64> name = {};
    bool is_self_hosted = false;
};

char *append_byte(char *buf, char val)
{
    *(buf++) = val;
    return buf;
}

char *append_bool(char *buf, bool val)
{
    uint8_t n_val = (uint8_t)val;
    *(buf++) = n_val;
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
char *read_bool(char *buf, bool *val)
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

char *read_uint32(char *buf, uint32_t *val)
{
    uint32_t n_val = *(uint32_t *)buf;
    *val = ntohl(n_val);
    return buf + 4;
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

struct MessageBuilder
{
    char data_buf[MAX_MSG_SIZE];
    char *data;

    MessageBuilder()
    {
        // skip space for size
        data = data_buf + sizeof(uint16_t);
    }

    MessageBuilder(char header_type)
    {
        // skip space for size and add header type
        data = data_buf + sizeof(uint16_t);
        append(header_type);
    }

    void reset()
    {
        data = data_buf + sizeof(uint16_t);
    }

    void reset(char header_type)
    {
        data = data_buf + sizeof(uint16_t);
        append(header_type);
    }

    void append(char val)
    {
        *(data++) = val;
    }

    void append(uint8_t val)
    {
        *(data++) = val;
    }

    void append(bool val)
    {
        uint8_t n_val = (uint8_t)val;
        *(data++) = n_val;
    }
    void append(uint16_t val)
    {
        uint16_t n_val = htons(val);
        *(data++) = n_val;
        *(data++) = n_val >> 8;
    }
    void append(uint32_t val)
    {
        uint32_t n_val = htonl(val);
        *(uint32_t *)data = n_val;
        data += sizeof(uint32_t);
    }
    void append(uint64_t val)
    {
        uint64_t n_val = htonll(val);
        *(uint64_t *)data = n_val;
        data += sizeof(uint64_t);
    }
    void append(char *str, uint16_t len)
    {
        append(len);

        memcpy(data, str, len);
        data += len;
    }
    void append(String str)
    {
        append(str.len);

        memcpy(data, str.data, str.len);
        data += str.len;
    }

    uint16_t get_len()
    {
        return data - data_buf;
    }

    void send(Peer *peer)
    {
        uint16_t len = get_len();

        uint16_t n_len = htons(len);
        data_buf[0] = n_len;
        data_buf[1] = n_len >> 8;

        peer->send_all(data_buf, len);
    }
};

struct MessageReader
{
    char data_buf[MAX_MSG_SIZE];
    char *data;

    char *end;

    MessageReader(char *data, uint16_t len)
    {
        this->data = this->data_buf;
        this->end = this->data_buf;
        if (len <= MAX_MSG_SIZE)
        {
            memcpy(this->data_buf, data, len);
            this->end = this->data_buf + len;
        }
    }

    void check(char *ptr)
    {
        if (ptr > end)
        {
            // gone past the end of the message
            assert(false);
            exit(1);
        }
    }

    uint32_t read(char *val)
    {
        check(data + 1);
        *val = data[0];
        data++;

        return 1;
    }

    uint32_t read(uint8_t *val)
    {
        check(data + 1);
        *val = data[0];
        data++;

        return 1;
    }

    uint32_t read(bool *val)
    {
        check(data + 1);
        *val = data[0];
        data++;

        return 1;
    }

    uint32_t read(uint16_t *val)
    {
        check(data + 2);
        uint16_t n_val = *(uint16_t *)data;
        *val = ntohs(n_val);
        data += 2;

        return 2;
    }

    uint32_t read(uint32_t *val)
    {
        check(data + 4);
        uint32_t n_val = *(uint32_t *)data;
        *val = ntohl(n_val);
        data += 4;

        return 4;
    }

    uint32_t read(uint64_t *val)
    {
        check(data + 8);
        uint64_t n_val = *(uint64_t *)data;
        *val = ntohll(n_val);
        data += 8;

        return 8;
    }

    uint32_t read(char *output_buf, uint16_t *len)
    {
        read(data, len);

        check(data + *len);
        memcpy(output_buf, data, *len);
        data += *len;

        return *len;
    }

    uint32_t read(String *output)
    {
        read(&output->len);

        check(data + output->len);
        memcpy(output->data, data, output->len);
        data += output->len;

        return output->len;
    }

    // returns pointer to string within buf, no copying
    uint32_t read_string_inplace(char **val, uint16_t *len)
    {
        read(data, len);

        check(data + *len);
        *val = data;
        data += *len;
        
        return *len;
    }
};

struct Client
{
    Peer peer = {};

    ClientId client_id = 0;
    GameId game_id = 0;

    bool ready = false;

    AllocatedString<32> username;
    void set_username(String str)
    {
        username.len = 0;
        for (int i = 0; i < str.len && i < username.MAX_LEN; i++)
        {
            if (str.data[i] > 32 && str.data[i] < 127)
            {
                username.data[username.len] = str.data[i];
                username.len++;
            }
        }
    }
};