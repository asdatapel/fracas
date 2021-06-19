
#pragma once

#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcClient : public BaseRpcClient
{
    using BaseRpcClient::BaseRpcClient;
    bool handle_rpc(char*, int);

    void HandleStartGame(StartGameRequest*);


    void ListGames(ListGamesRequest &, ListGamesResponse*);
    ListGamesResponse ListGames(ListGamesRequest);

    void GetGame(GetGameRequest &, GetGameResponse*);
    GetGameResponse GetGame(GetGameRequest);

    void CreateGame(CreateGameRequest &, CreateGameResponse*);
    CreateGameResponse CreateGame(CreateGameRequest);

    void JoinGame(JoinGameRequest &, JoinGameResponse*);
    JoinGameResponse JoinGame(JoinGameRequest);

    void SwapTeam(SwapTeamRequest &, SwapTeamResponse*);
    SwapTeamResponse SwapTeam(SwapTeamRequest);

    void LeaveGame(LeaveGameRequest &, LeaveGameResponse*);
    LeaveGameResponse LeaveGame(LeaveGameRequest);

    void StartGame(StartGameRequest &, StartGameResponse*);
    StartGameResponse StartGame(StartGameRequest);

};

bool RpcClient::handle_rpc(char *data, int msg_len)
{
    Rpc rpc_type;
    data = read_byte(data, (char *)&rpc_type);
    MessageReader in(data, msg_len - 1);
    switch(rpc_type) {
    case Rpc::OnewayStartGame:
        {
            StartGameRequest req;
            read(&in, &req);
            HandleStartGame(&req);
        }
        break;
        default:
        return false;
    }

    return true;
}


void RpcClient::ListGames(ListGamesRequest &req, ListGamesResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::ListGames);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::ListGames){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
ListGamesResponse RpcClient::ListGames(ListGamesRequest req) 
{
    ListGamesResponse resp;
    ListGames(req, &resp);
    return resp;
}


void RpcClient::GetGame(GetGameRequest &req, GetGameResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::GetGame);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::GetGame){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
GetGameResponse RpcClient::GetGame(GetGameRequest req) 
{
    GetGameResponse resp;
    GetGame(req, &resp);
    return resp;
}


void RpcClient::CreateGame(CreateGameRequest &req, CreateGameResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::CreateGame);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::CreateGame){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
CreateGameResponse RpcClient::CreateGame(CreateGameRequest req) 
{
    CreateGameResponse resp;
    CreateGame(req, &resp);
    return resp;
}


void RpcClient::JoinGame(JoinGameRequest &req, JoinGameResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::JoinGame);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::JoinGame){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
JoinGameResponse RpcClient::JoinGame(JoinGameRequest req) 
{
    JoinGameResponse resp;
    JoinGame(req, &resp);
    return resp;
}


void RpcClient::SwapTeam(SwapTeamRequest &req, SwapTeamResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::SwapTeam);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::SwapTeam){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
SwapTeamResponse RpcClient::SwapTeam(SwapTeamRequest req) 
{
    SwapTeamResponse resp;
    SwapTeam(req, &resp);
    return resp;
}


void RpcClient::LeaveGame(LeaveGameRequest &req, LeaveGameResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::LeaveGame);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::LeaveGame){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
LeaveGameResponse RpcClient::LeaveGame(LeaveGameRequest req) 
{
    LeaveGameResponse resp;
    LeaveGame(req, &resp);
    return resp;
}


void RpcClient::StartGame(StartGameRequest &req, StartGameResponse *resp)
{
    MessageBuilder out;
    append(&out, (char) Rpc::StartGame);
    append(&out, req); out.send(&peer);
    
    int msg_len; char msg[MAX_MSG_SIZE];
    while ((msg_len = peer.recieve_msg(msg)) < 0){}
    
    if (msg_len > 0 && msg[0] == (char) Rpc::StartGame){
        peer.pop_message();
        MessageReader in(msg + 1, msg_len - 1);
        read(&in, resp);
    }
}
StartGameResponse RpcClient::StartGame(StartGameRequest req) 
{
    StartGameResponse resp;
    StartGame(req, &resp);
    return resp;
}

