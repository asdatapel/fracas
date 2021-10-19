#include <chrono>
#include <stdio.h>
#include <stdint.h>
#include <map>

#include "net/net.cpp"

#include <winsock2.h>

#include "common.hpp"
#include "game_state.hpp"
#include "lobby.hpp"
#include "net/net.hpp"

SOCKET open_socket(uint16_t port)
{
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
    server.sin_port = htons(port);
    if (bind(s, (sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d", WSAGetLastError());
    }
    listen(s, 3);

    return s;
}

int main(int argc, char *argv[])
{
    init_net();
    ServerData server_data;

    SOCKET s = open_socket(6519);
    SOCKET rpc_socket = open_socket(6666);

    RpcServer rpc_server{&server_data};

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
                closesocket(new_socket);
            }
        }

        {
            sockaddr_in client = {};
            int c = sizeof(sockaddr_in);
            SOCKET new_socket = accept(rpc_socket, (sockaddr *)&client, &c);
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
                add_client(&server_data, new_socket);
            }
        }

        ClientId client_to_delete = 0; // delete one at a time :shrug:
        for (auto &it : server_data.clients)
        {
            ClientId client_id = it.first;
            Client *client = &(it.second);

            if (!client->peer.is_connected())
            {
                rpc_server.on_disconnect(client_id);
                client_to_delete = client_id;
                continue;
            }

            int msg_len;
            char msg[MAX_MSG_SIZE];
            while ((msg_len = client->peer.recieve_msg(msg)) > 0)
            {
                client->peer.pop_message();
                rpc_server.handle_rpc(client->client_id, &client->peer, msg, msg_len);
            }
        }
        if(client_to_delete)
        {
            server_data.clients.erase(client_to_delete);
        }
        
        for (auto& it : server_data.lobbies)
        {
            Lobby *lobby = &it.second;
            lobby->tick({&rpc_server, lobby}, elapsed.count());
            if (lobby->stage == LobbyStage::DEAD)
            {
                server_data.lobbies.erase(it.first);
            }
        }

        // printf("Sleeping...\n");
        Sleep(1000);
    }

    closesocket(s);
    deinit_net();

    return 0;
}