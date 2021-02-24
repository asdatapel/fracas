#include <stdio.h>
#include <stdint.h>

#include <winsock2.h>

#include "game_state.hpp"
#include "net.hpp"

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char *argv[])
{
    GameState game;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    SOCKET s;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
    }
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, (unsigned long *)&mode);

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(6519);
    if (bind(s, (sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d", WSAGetLastError());
    }

    listen(s, 3);

    while (true)
    {
        {
            SOCKET new_socket;
            sockaddr_in client;
            int c = sizeof(sockaddr_in);
            new_socket = accept(s, (sockaddr *)&client, &c);
            if (new_socket == INVALID_SOCKET)
            {
                int err;
                if ((err = WSAGetLastError()) != WSAEWOULDBLOCK)
                {
                    printf("accept failed with error code : %d\n", err);
                }
            }
            else
            {
                if (game.clients.append({new_socket}) != -1)
                {
                    build_and_broadcast_msg(game, construct_msg_describe_lobby);
                }
            }
        }

        for (int i = 0; i < game.clients.len; i++)
        {
            Client &client = game.clients[i];
            if (client.peer.socket == 0)
            {
                if (game.round == -1) // if game hasn't started
                {
                    game.clients.swap_delete(i);
                }
                continue;
            }

            int msg_len;
            char msg[MAX_MSG_SIZE];
            while ((msg_len = client.peer.recieve_msg(msg)) > 0)
            {
                ClientMessageType msg_type;
                char *data = read_byte(msg, (char *)&msg_type);

                if (msg_type >= ClientMessageType::INVALID)
                    DEBUG_PRINT("There has been an error\n");

                HandleFunc handle_func = handle_funcs[(uint8_t)msg_type];
                handle_func(game, data, i);
            }
        }

        // printf("Sleeping...\n");
        // Sleep(1000);
    }

    closesocket(s);
    WSACleanup();

    return 0;
}