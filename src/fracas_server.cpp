#include <chrono>
#include <stdio.h>
#include <stdint.h>
#include <map>

#include <winsock2.h>

#include "common.hpp"
#include "game_state.hpp"
#include "lobby.hpp"
#include "net.hpp"

#pragma comment(lib, "ws2_32.lib")


typedef void (*HandleFunc)(MessageReader *, Client *);
HandleFunc handle_funcs[(int)ClientMessageType::INVALID] = {
    handle_LIST_GAMES,  // LIST_GAMES,
    handle_CREATE_GAME, // CREATE_GAME,
    handle_JOIN_GAME,   // JOIN_GAME,
    handle_LEAVE_GAME,  // LEAVE_GAME,
    
    handle_LEAVE_GAME,  
                        // READY,
                        // START,
                        // BUZZ,
                        // PASS_OR_PLAY,
                        // ANSWER,

                        // HOST_START_ASK_QUESTION,
                        // HOST_RESPOND_TO_ANSWER,
                        // HOST_END_GAME,

                        // CELEBRATE,
};

int main(int argc, char *argv[])
{
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

    auto loop_start_time = std::chrono::high_resolution_clock::now();
    while (true)
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = now - loop_start_time;
        loop_start_time = now;

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
                add_client(new_socket);
            }
        }

        for (auto it : clients)
        {
            ClientId client_id = it.first;
            Client *client = &it.second;
            if (client->peer.socket == 0)
            {
                remove_client(client_id);
                continue;
            }

            int msg_len;
            char msg[MAX_MSG_SIZE];
            while ((msg_len = client->peer.recieve_msg(msg)) > 0)
            {
                ClientMessageType msg_type;
                char *data = read_byte(msg, (char *)&msg_type);
                MessageReader msg(data, msg_len - 1);

                if (msg_type >= ClientMessageType::INVALID)
                    DEBUG_PRINT("There has been an error\n");

                HandleFunc handle_func = handle_funcs[(uint8_t)msg_type];
                handle_func(&msg, client);
            }
        }

        for (auto it : games)
        {
            GameState *game = &it.second;
            //server_tick(game, elapsed.count());
            if (game->stage == GameStage::DEAD)
            {
                games.erase(it.first);
            }
        }

        // printf("Sleeping...\n");
        // Sleep(1000);
    }

    closesocket(s);
    WSACleanup();

    return 0;
}