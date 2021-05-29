#include <stdint.h>

#include "net.hpp"
#include "net_windows.hpp"

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

MessageBuilder::MessageBuilder()
{
    // skip space for size
    data = data_buf + sizeof(uint16_t);
}
MessageBuilder::MessageBuilder(char header_type)
{
    // skip space for size and add header type
    data = data_buf + sizeof(uint16_t);
    append(this, header_type);
}
void MessageBuilder::reset()
{
    data = data_buf + sizeof(uint16_t);
}
void MessageBuilder::reset(char header_type)
{
    data = data_buf + sizeof(uint16_t);
    append(this, header_type);
}
uint16_t MessageBuilder::get_len()
{
    return data - data_buf;
}
void MessageBuilder::send(Peer *peer)
{
    uint16_t len = get_len();

    uint16_t n_len = htons(len);
    data_buf[0] = n_len;
    data_buf[1] = n_len >> 8;

    peer->send_all(data_buf, len);
}

void append(MessageBuilder *msg, char val)
{
    *(msg->data++) = val;
}

void append(MessageBuilder *msg, uint8_t val)
{
    *(msg->data++) = val;
}

void append(MessageBuilder *msg, bool val)
{
    uint8_t n_val = (uint8_t)val;
    *(msg->data++) = n_val;
}
void append(MessageBuilder *msg, uint16_t val)
{
    uint16_t n_val = htons(val);
    *(msg->data++) = n_val;
    *(msg->data++) = n_val >> 8;
}
void append(MessageBuilder *msg, uint32_t val)
{
    uint32_t n_val = htonl(val);
    *(uint32_t *)msg->data = n_val;
    msg->data += sizeof(uint32_t);
}
void append(MessageBuilder *msg, int32_t val)
{
    uint32_t n_val = htonl(val);
    *(uint32_t *)msg->data = n_val;
    msg->data += sizeof(uint32_t);
}
void append(MessageBuilder *msg, uint64_t val)
{
    uint64_t n_val = htonll(val);
    *(uint64_t *)msg->data = n_val;
    msg->data += sizeof(uint64_t);
}
void append(MessageBuilder *msg, char *str, uint16_t len)
{
    append(msg, len);

    memcpy(msg->data, str, len);
    msg->data += len;
}
void append(MessageBuilder *msg, String str)
{
    append(msg, str.len);

    memcpy(msg->data, str.data, str.len);
    msg->data += str.len;
}

MessageReader::MessageReader(char *data, uint16_t len)
{
    this->data = this->data_buf;
    this->end = this->data_buf;
    if (len <= MAX_MSG_SIZE)
    {
        memcpy(this->data_buf, data, len);
        this->end = this->data_buf + len;
    }
}
void MessageReader::check(char *ptr)
{
    if (ptr > end)
    {
        // gone past the end of the message
        assert(false);
        exit(1);
    }
}

uint32_t read(MessageReader *msg, char *val)
{
    msg->check(msg->data + 1);
    *val = msg->data[0];
    msg->data++;

    return 1;
}
uint32_t read(MessageReader *msg, uint8_t *val)
{
    msg->check(msg->data + 1);
    *val = msg->data[0];
    msg->data++;

    return 1;
}
uint32_t read(MessageReader *msg, bool *val)
{
    msg->check(msg->data + 1);
    *val = msg->data[0];
    msg->data++;

    return 1;
}
uint32_t read(MessageReader *msg, uint16_t *val)
{
    msg->check(msg->data + 2);
    uint16_t n_val = *(uint16_t *)msg->data;
    *val = ntohs(n_val);
    msg->data += 2;

    return 2;
}
uint32_t read(MessageReader *msg, uint32_t *val)
{
    msg->check(msg->data + 4);
    uint32_t n_val = *(uint32_t *)msg->data;
    *val = ntohl(n_val);
    msg->data += 4;

    return 4;
}
uint32_t read(MessageReader *msg, int32_t *val)
{
    msg->check(msg->data + 4);
    uint32_t n_val = *(uint32_t *)msg->data;
    *val = ntohl(n_val);
    msg->data += 4;

    return 4;
}
uint32_t read(MessageReader *msg, uint64_t *val)
{
    msg->check(msg->data + 8);
    uint64_t n_val = *(uint64_t *)msg->data;
    *val = ntohll(n_val);
    msg->data += 8;

    return 8;
}
uint32_t read(MessageReader *msg, char *output_buf, uint16_t *len)
{
    read(msg, len);

    msg->check(msg->data + *len);
    memcpy(output_buf, msg->data, *len);
    msg->data += *len;

    return *len;
}
uint32_t read(MessageReader *msg, String *output)
{
    read(msg, &output->len);

    msg->check(msg->data + output->len);
    output->data = msg->data;
    msg->data += output->len;

    return output->len;
}
uint32_t read_string_inplace(MessageReader *msg, char **val, uint16_t *len)
{
    read(msg, len);

    msg->check(msg->data + *len);
    *val = msg->data;
    msg->data += *len;

    return *len;
}

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