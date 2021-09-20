
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
    void HandleSwapTeam(ClientId client_id, SwapTeamRequest*, Empty*);
    void HandleLeaveGame(ClientId client_id, LeaveGameRequest*, LeaveGameResponse*);
    void HandleStartGame(ClientId client_id, StartGameRequest*, StartGameResponse*);


    void GameStarted(Peer *, GameStartedMessage);

    void InGameStartRound(Peer *, Empty);

    void InGameStartFaceoff(Peer *, Empty);

    void InGameAskQuestion(Peer *, Empty);

    void InGamePromptPassOrPlay(Peer *, Empty);

    void InGamePlayerBuzzed(Peer *, Empty);

    void InGamePromptForAnswer(Peer *, Empty);

    void InGameStartPlay(Peer *, Empty);

    void InGameAnswer(Peer *, Empty);

    void InGameFlipAnswer(Peer *, Empty);

    void InGameEggghhhh(Peer *, Empty);

    void InGameEndRound(Peer *, Empty);

    void InGameEndGame(Peer *, Empty);

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
        Empty resp;
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
    default:
        assert(false);
    }
}


void RpcServer::GameStarted(Peer *peer, GameStartedMessage req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::GameStarted);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameStartRound(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameStartRound);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameStartFaceoff(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameStartFaceoff);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameAskQuestion(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameAskQuestion);
    append(&out, req); out.send(peer);
}


void RpcServer::InGamePromptPassOrPlay(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGamePromptPassOrPlay);
    append(&out, req); out.send(peer);
}


void RpcServer::InGamePlayerBuzzed(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGamePlayerBuzzed);
    append(&out, req); out.send(peer);
}


void RpcServer::InGamePromptForAnswer(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGamePromptForAnswer);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameStartPlay(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameStartPlay);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameAnswer(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameAnswer);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameFlipAnswer(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameFlipAnswer);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameEggghhhh(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameEggghhhh);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameEndRound(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameEndRound);
    append(&out, req); out.send(peer);
}


void RpcServer::InGameEndGame(Peer *peer, Empty req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::InGameEndGame);
    append(&out, req); out.send(peer);
}

