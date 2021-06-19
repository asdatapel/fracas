#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>

#include "net.hpp"

#pragma comment(lib, "ws2_32.lib")

WSADATA wsa;

void init_net()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        assert(false);
    }
}

void deinit_net()
{
    WSACleanup();
}

struct Peer
{
    SOCKET s;

    char msg_buf[MAX_MSG_SIZE * 2];
    uint32_t received_so_far = 0;

    void open(const char *address, uint16_t port, bool blocking)
    {
        if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        {
            printf("Could not create socket : %d", WSAGetLastError());
            assert(false);
        }

        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        inet_pton(AF_INET, address, &server_address.sin_addr.S_un.S_addr);
        server_address.sin_port = htons(port);
        if (connect(s, (sockaddr *)&server_address, sizeof(server_address)) != 0)
        {
            printf("Could not connect to server : %d", WSAGetLastError());
            assert(false);
        }

        u_long mode = blocking ? 0 : 1;
        ioctlsocket(s, FIONBIO, (unsigned long *)&mode);
    }

    void close()
    {
        closesocket(s);
    }

    bool is_connected()
    {
        return s != 0;
    }

    void pop_message()
    {
        uint16_t len;
        char *data = read_short(msg_buf, &len);
        received_so_far -= len;
        memcpy(msg_buf, &msg_buf[len], received_so_far);
    }

    int recieve_msg(char *dst)
    {
        if (received_so_far >= 2)
        {
            uint16_t expected_len;
            char *data = read_short(msg_buf, &expected_len);

            if (received_so_far >= expected_len)
            {
                memcpy(dst, data, expected_len - 2);
                return expected_len - 2;
            }
        }

        int received_this_time = recv(s, msg_buf + received_so_far,
                                      (MAX_MSG_SIZE * 2) - received_so_far, 0);
        if (received_this_time == 0)
        {
            s = 0;
            return -1;
        }
        if (received_this_time == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK)
            {
                printf("Failure trying to recieve data : %d\n", err);

                if (err == WSAECONNRESET || err == WSAECONNRESET)
                {
                    s = 0;
                }
                return -1;
            }
            return -1;
        }

        received_so_far += received_this_time;

        return -1;
    }

    void send_all(char *msg, uint16_t len)
    {
        int sent = 0;
        while (sent < len)
        {
            int sent_this_time;
            if ((sent_this_time = send(s, msg + sent, len - sent, 0)) == SOCKET_ERROR)
            {
                printf("Failure trying to send data, error : %d, already sent: %d, remaining: %d\n", WSAGetLastError(), sent, len - sent);
                break;
            }
            sent += sent_this_time;
        }
    }
};