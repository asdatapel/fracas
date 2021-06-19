
#pragma once

#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcServer : public BaseRpcServer
{
    using BaseRpcServer::BaseRpcServer;
    void handle_rpc(ClientId, Peer*, char*, int);

    void HandleListGames(ClientId client_id, ListGamesRequest*, ListGamesResponse*);
    void HandleGetGame(ClientId client_id, GetGameRequest*, GetGameResponse*);
    void HandleCreateGame(ClientId client_id, CreateGameRequest*, CreateGameResponse*);
    void HandleJoinGame(ClientId client_id, JoinGameRequest*, JoinGameResponse*);
    void HandleSwapTeam(ClientId client_id, SwapTeamRequest*, SwapTeamResponse*);
    void HandleLeaveGame(ClientId client_id, LeaveGameRequest*, LeaveGameResponse*);
    void HandleStartGame(ClientId client_id, StartGameRequest*, StartGameResponse*);


    void StartGame(Peer *, StartGameRequest);

};

void RpcServer::handle_rpc(ClientId client_id, Peer *peer, char *data, int msg_len)
{
    Rpc rpc_type;
    data = read_byte(data, (char *)&rpc_type);
    MessageReader in(data, msg_len - 1);
    MessageBuilder out;
    switch(rpc_type) {
    case Rpc::ListGames:
        {
            ListGamesRequest req;
            ListGamesResponse resp;
            read(&in, &req);
            HandleListGames(client_id, &req, &resp);
            append(&out, (char)Rpc::ListGames);
            append(&out, resp);
            out.send(peer);
        }
        break;
    case Rpc::GetGame:
        {
            GetGameRequest req;
            GetGameResponse resp;
            read(&in, &req);
            HandleGetGame(client_id, &req, &resp);
            append(&out, (char)Rpc::GetGame);
            append(&out, resp);
            out.send(peer);
        }
        break;
    case Rpc::CreateGame:
        {
            CreateGameRequest req;
            CreateGameResponse resp;
            read(&in, &req);
            HandleCreateGame(client_id, &req, &resp);
            append(&out, (char)Rpc::CreateGame);
            append(&out, resp);
            out.send(peer);
        }
        break;
    case Rpc::JoinGame:
        {
            JoinGameRequest req;
            JoinGameResponse resp;
            read(&in, &req);
            HandleJoinGame(client_id, &req, &resp);
            append(&out, (char)Rpc::JoinGame);
            append(&out, resp);
            out.send(peer);
        }
        break;
    case Rpc::SwapTeam:
        {
            SwapTeamRequest req;
            SwapTeamResponse resp;
            read(&in, &req);
            HandleSwapTeam(client_id, &req, &resp);
            append(&out, (char)Rpc::SwapTeam);
            append(&out, resp);
            out.send(peer);
        }
        break;
    case Rpc::LeaveGame:
        {
            LeaveGameRequest req;
            LeaveGameResponse resp;
            read(&in, &req);
            HandleLeaveGame(client_id, &req, &resp);
            append(&out, (char)Rpc::LeaveGame);
            append(&out, resp);
            out.send(peer);
        }
        break;
    case Rpc::StartGame:
        {
            StartGameRequest req;
            StartGameResponse resp;
            read(&in, &req);
            HandleStartGame(client_id, &req, &resp);
            append(&out, (char)Rpc::StartGame);
            append(&out, resp);
            out.send(peer);
        }
        break;
    }
}


void RpcServer::StartGame(Peer *peer, StartGameRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::OnewayStartGame);
    append(&out, req); out.send(peer);
}

