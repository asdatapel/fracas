#pragma once

#include "net_windows.hpp"

struct ServerData;
struct BaseRpcServer
{
    ServerData *server_data;
    BaseRpcServer(ServerData *server_data)
    {
        this->server_data = server_data;
    }
};

struct ClientData;
struct BaseRpcClient
{
    ClientData *client_data;
    Peer peer;
    BaseRpcClient(const char *address, uint16_t port)
    {
        peer.open(address, port, false);
    }
};