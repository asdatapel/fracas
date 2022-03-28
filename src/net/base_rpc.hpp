#pragma once

#define NOMINMAX
#include "net_windows.hpp"

struct ServerData;
struct BaseRpcServer {
  ServerData *server_data;
  BaseRpcServer(ServerData *server_data) { this->server_data = server_data; }

  void on_disconnect(ClientId client_id);
};

struct ClientData;
struct BaseRpcClient {
  ClientData *client_data;
  Peer peer;
  BaseRpcClient() = default;
  BaseRpcClient(const char *address, uint16_t port) { peer.open(address, port, false); }

  void connect(const char *address, uint16_t port)
  {
    if (peer.is_connected()) {
      peer.close();
    }
    peer.open(address, port, false);
  }
};

struct Lobby;
struct Broadcaster {
  BaseRpcServer *rpc_server;
  Lobby *lobby;

  template <typename FN, typename MSG>
  void broadcast(FN, MSG);
};