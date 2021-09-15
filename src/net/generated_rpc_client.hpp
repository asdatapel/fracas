#pragma once

#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcClient : public BaseRpcClient
{
    using BaseRpcClient::BaseRpcClient;
    bool handle_rpc(char*, int);


    void ListGames(ListGamesRequest);

    void GetGame(GetGameRequest);

    void CreateGame(CreateGameRequest);

    void JoinGame(JoinGameRequest);

    void SwapTeam(SwapTeamRequest);

    void LeaveGame(LeaveGameRequest);

    void StartGame(StartGameRequest);



    ListGamesResponse *get_ListGames_msg();
    bool got_ListGames_msg = false;
    ListGamesResponse ListGames_msg;

    GetGameResponse *get_GetGame_msg();
    bool got_GetGame_msg = false;
    GetGameResponse GetGame_msg;

    CreateGameResponse *get_CreateGame_msg();
    bool got_CreateGame_msg = false;
    CreateGameResponse CreateGame_msg;

    JoinGameResponse *get_JoinGame_msg();
    bool got_JoinGame_msg = false;
    JoinGameResponse JoinGame_msg;

    Empty *get_SwapTeam_msg();
    bool got_SwapTeam_msg = false;
    Empty SwapTeam_msg;

    LeaveGameResponse *get_LeaveGame_msg();
    bool got_LeaveGame_msg = false;
    LeaveGameResponse LeaveGame_msg;

    StartGameResponse *get_StartGame_msg();
    bool got_StartGame_msg = false;
    StartGameResponse StartGame_msg;

    GameStartedMessage *get_GameStarted_msg();
    bool got_GameStarted_msg = false;
    GameStartedMessage GameStarted_msg;

    Empty *get_InGameStartRound_msg();
    bool got_InGameStartRound_msg = false;
    Empty InGameStartRound_msg;

    Empty *get_InGameStartFaceoff_msg();
    bool got_InGameStartFaceoff_msg = false;
    Empty InGameStartFaceoff_msg;

    Empty *get_InGameAskQuestion_msg();
    bool got_InGameAskQuestion_msg = false;
    Empty InGameAskQuestion_msg;

    Empty *get_InGamePromptPassOrPlay_msg();
    bool got_InGamePromptPassOrPlay_msg = false;
    Empty InGamePromptPassOrPlay_msg;

    Empty *get_InGameAnswer_msg();
    bool got_InGameAnswer_msg = false;
    Empty InGameAnswer_msg;

    Empty *get_InGameFlipAnswer_msg();
    bool got_InGameFlipAnswer_msg = false;
    Empty InGameFlipAnswer_msg;

};

bool RpcClient::handle_rpc(char *data, int msg_len)
{
    Rpc rpc_type;
    data = read_byte(data, (char *)&rpc_type);
    MessageReader in(data, msg_len - 1);
    switch(rpc_type) {

        case Rpc::ListGames:
        {
            ListGames_msg = {};
            read(&in, &ListGames_msg);
            got_ListGames_msg = true;
        }
        break;
        case Rpc::GetGame:
        {
            GetGame_msg = {};
            read(&in, &GetGame_msg);
            got_GetGame_msg = true;
        }
        break;
        case Rpc::CreateGame:
        {
            CreateGame_msg = {};
            read(&in, &CreateGame_msg);
            got_CreateGame_msg = true;
        }
        break;
        case Rpc::JoinGame:
        {
            JoinGame_msg = {};
            read(&in, &JoinGame_msg);
            got_JoinGame_msg = true;
        }
        break;
        case Rpc::SwapTeam:
        {
            SwapTeam_msg = {};
            read(&in, &SwapTeam_msg);
            got_SwapTeam_msg = true;
        }
        break;
        case Rpc::LeaveGame:
        {
            LeaveGame_msg = {};
            read(&in, &LeaveGame_msg);
            got_LeaveGame_msg = true;
        }
        break;
        case Rpc::StartGame:
        {
            StartGame_msg = {};
            read(&in, &StartGame_msg);
            got_StartGame_msg = true;
        }
        break;
        case Rpc::GameStarted:
        {
            GameStarted_msg = {};
            read(&in, &GameStarted_msg);
            got_GameStarted_msg = true;
        }
        break;
        case Rpc::InGameStartRound:
        {
            InGameStartRound_msg = {};
            read(&in, &InGameStartRound_msg);
            got_InGameStartRound_msg = true;
        }
        break;
        case Rpc::InGameStartFaceoff:
        {
            InGameStartFaceoff_msg = {};
            read(&in, &InGameStartFaceoff_msg);
            got_InGameStartFaceoff_msg = true;
        }
        break;
        case Rpc::InGameAskQuestion:
        {
            InGameAskQuestion_msg = {};
            read(&in, &InGameAskQuestion_msg);
            got_InGameAskQuestion_msg = true;
        }
        break;
        case Rpc::InGamePromptPassOrPlay:
        {
            InGamePromptPassOrPlay_msg = {};
            read(&in, &InGamePromptPassOrPlay_msg);
            got_InGamePromptPassOrPlay_msg = true;
        }
        break;
        case Rpc::InGameAnswer:
        {
            InGameAnswer_msg = {};
            read(&in, &InGameAnswer_msg);
            got_InGameAnswer_msg = true;
        }
        break;
        case Rpc::InGameFlipAnswer:
        {
            InGameFlipAnswer_msg = {};
            read(&in, &InGameFlipAnswer_msg);
            got_InGameFlipAnswer_msg = true;
        }
        break;
        default:
        return false;
    }

    return true;
}


void RpcClient::ListGames(ListGamesRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::ListGames);
    append(&out, req); out.send(&peer);
}

void RpcClient::GetGame(GetGameRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::GetGame);
    append(&out, req); out.send(&peer);
}

void RpcClient::CreateGame(CreateGameRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::CreateGame);
    append(&out, req); out.send(&peer);
}

void RpcClient::JoinGame(JoinGameRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::JoinGame);
    append(&out, req); out.send(&peer);
}

void RpcClient::SwapTeam(SwapTeamRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::SwapTeam);
    append(&out, req); out.send(&peer);
}

void RpcClient::LeaveGame(LeaveGameRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::LeaveGame);
    append(&out, req); out.send(&peer);
}

void RpcClient::StartGame(StartGameRequest req)
{
    MessageBuilder out;
    append(&out, (char) Rpc::StartGame);
    append(&out, req); out.send(&peer);
}



ListGamesResponse *RpcClient::get_ListGames_msg()
{
    auto msg = got_ListGames_msg ? &ListGames_msg : nullptr;
    got_ListGames_msg = false;
    return msg;
}

GetGameResponse *RpcClient::get_GetGame_msg()
{
    auto msg = got_GetGame_msg ? &GetGame_msg : nullptr;
    got_GetGame_msg = false;
    return msg;
}

CreateGameResponse *RpcClient::get_CreateGame_msg()
{
    auto msg = got_CreateGame_msg ? &CreateGame_msg : nullptr;
    got_CreateGame_msg = false;
    return msg;
}

JoinGameResponse *RpcClient::get_JoinGame_msg()
{
    auto msg = got_JoinGame_msg ? &JoinGame_msg : nullptr;
    got_JoinGame_msg = false;
    return msg;
}

Empty *RpcClient::get_SwapTeam_msg()
{
    auto msg = got_SwapTeam_msg ? &SwapTeam_msg : nullptr;
    got_SwapTeam_msg = false;
    return msg;
}

LeaveGameResponse *RpcClient::get_LeaveGame_msg()
{
    auto msg = got_LeaveGame_msg ? &LeaveGame_msg : nullptr;
    got_LeaveGame_msg = false;
    return msg;
}

StartGameResponse *RpcClient::get_StartGame_msg()
{
    auto msg = got_StartGame_msg ? &StartGame_msg : nullptr;
    got_StartGame_msg = false;
    return msg;
}

GameStartedMessage *RpcClient::get_GameStarted_msg()
{
    auto msg = got_GameStarted_msg ? &GameStarted_msg : nullptr;
    got_GameStarted_msg = false;
    return msg;
}

Empty *RpcClient::get_InGameStartRound_msg()
{
    auto msg = got_InGameStartRound_msg ? &InGameStartRound_msg : nullptr;
    got_InGameStartRound_msg = false;
    return msg;
}

Empty *RpcClient::get_InGameStartFaceoff_msg()
{
    auto msg = got_InGameStartFaceoff_msg ? &InGameStartFaceoff_msg : nullptr;
    got_InGameStartFaceoff_msg = false;
    return msg;
}

Empty *RpcClient::get_InGameAskQuestion_msg()
{
    auto msg = got_InGameAskQuestion_msg ? &InGameAskQuestion_msg : nullptr;
    got_InGameAskQuestion_msg = false;
    return msg;
}

Empty *RpcClient::get_InGamePromptPassOrPlay_msg()
{
    auto msg = got_InGamePromptPassOrPlay_msg ? &InGamePromptPassOrPlay_msg : nullptr;
    got_InGamePromptPassOrPlay_msg = false;
    return msg;
}

Empty *RpcClient::get_InGameAnswer_msg()
{
    auto msg = got_InGameAnswer_msg ? &InGameAnswer_msg : nullptr;
    got_InGameAnswer_msg = false;
    return msg;
}

Empty *RpcClient::get_InGameFlipAnswer_msg()
{
    auto msg = got_InGameFlipAnswer_msg ? &InGameFlipAnswer_msg : nullptr;
    got_InGameFlipAnswer_msg = false;
    return msg;
}

