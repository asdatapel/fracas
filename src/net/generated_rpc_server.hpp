
#pragma once

#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcServer : public BaseRpcServer {
  using BaseRpcServer::BaseRpcServer;
  void handle_rpc(ClientId, Peer *, char *, int);

  void HandleListGames(ClientId client_id, ListGamesRequest *, ListGamesResponse *);
  void HandleGetGame(ClientId client_id, GetGameRequest *, GetGameResponse *);
  void HandleCreateGame(ClientId client_id, CreateGameRequest *, CreateGameResponse *);
  void HandleJoinGame(ClientId client_id, JoinGameRequest *, JoinGameResponse *);
  void HandleSwapTeam(ClientId client_id, SwapTeamRequest *, Empty *);
  void HandleLeaveGame(ClientId client_id, LeaveGameRequest *, LeaveGameResponse *);
  void HandleStartGame(ClientId client_id, StartGameRequest *, StartGameResponse *);
  void HandleInGameReady(ClientId client_id, Empty *, Empty *);
  void HandleInGameAnswer(ClientId client_id, InGameAnswerMessage *, Empty *);
  void HandleInGameBuzz(ClientId client_id, Empty *, Empty *);
  void HandleInGameChoosePassOrPlay(ClientId client_id, InGameChoosePassOrPlayMessage *, Empty *);

  void GameStarted(Peer *, GameStartedMessage);

  void PlayerLeft(Peer *, PlayerLeftMessage);

  void GameStatePing(Peer *, GameStatePingMessage);

  void InGameStartRound(Peer *, InGameStartRoundMessage);

  void InGameStartFaceoff(Peer *, InGameStartFaceoffMessage);

  void InGameAskQuestion(Peer *, InGameAskQuestionMessage);

  void InGamePromptPassOrPlay(Peer *, Empty);

  void InGamePlayerBuzzed(Peer *, InGamePlayerBuzzedMessage);

  void InGamePrepForPromptForAnswer(Peer *, InGamePrepForPromptForAnswerMessage);

  void InGamePromptForAnswer(Peer *, InGamePromptForAnswerMessage);

  void InGameStartPlay(Peer *, InGameStartPlayMessage);

  void InGameStartSteal(Peer *, InGameStartStealMessage);

  void InGamePlayerChosePassOrPlay(Peer *, InGameChoosePassOrPlayMessage);

  void InGamePlayerAnswered(Peer *, InGameAnswerMessage);

  void InGameFlipAnswer(Peer *, InGameFlipAnswerMessage);

  void InGameEggghhhh(Peer *, InGameEggghhhhMessage);

  void InGameEndRound(Peer *, InGameEndRoundMessage);

  void InGameEndGame(Peer *, InGameEndGameMessage);
};

void RpcServer::handle_rpc(ClientId client_id, Peer *peer, char *data, int msg_len)
{
  Rpc rpc_type;
  data = read_byte(data, (char *)&rpc_type);
  MessageReader in(data, msg_len - 1);
  MessageBuilder out;
  switch (rpc_type) {
    case Rpc::ListGames: {
      ListGamesRequest req;
      ListGamesResponse resp;
      read(&in, &req);
      HandleListGames(client_id, &req, &resp);
      append(&out, (char)Rpc::ListGames);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::GetGame: {
      GetGameRequest req;
      GetGameResponse resp;
      read(&in, &req);
      HandleGetGame(client_id, &req, &resp);
      append(&out, (char)Rpc::GetGame);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::CreateGame: {
      CreateGameRequest req;
      CreateGameResponse resp;
      read(&in, &req);
      HandleCreateGame(client_id, &req, &resp);
      append(&out, (char)Rpc::CreateGame);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::JoinGame: {
      JoinGameRequest req;
      JoinGameResponse resp;
      read(&in, &req);
      HandleJoinGame(client_id, &req, &resp);
      append(&out, (char)Rpc::JoinGame);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::SwapTeam: {
      SwapTeamRequest req;
      Empty resp;
      read(&in, &req);
      HandleSwapTeam(client_id, &req, &resp);
      append(&out, (char)Rpc::SwapTeam);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::LeaveGame: {
      LeaveGameRequest req;
      LeaveGameResponse resp;
      read(&in, &req);
      HandleLeaveGame(client_id, &req, &resp);
      append(&out, (char)Rpc::LeaveGame);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::StartGame: {
      StartGameRequest req;
      StartGameResponse resp;
      read(&in, &req);
      HandleStartGame(client_id, &req, &resp);
      append(&out, (char)Rpc::StartGame);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::InGameReady: {
      Empty req;
      Empty resp;
      read(&in, &req);
      HandleInGameReady(client_id, &req, &resp);
      append(&out, (char)Rpc::InGameReady);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::InGameAnswer: {
      InGameAnswerMessage req;
      Empty resp;
      read(&in, &req);
      HandleInGameAnswer(client_id, &req, &resp);
      append(&out, (char)Rpc::InGameAnswer);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::InGameBuzz: {
      Empty req;
      Empty resp;
      read(&in, &req);
      HandleInGameBuzz(client_id, &req, &resp);
      append(&out, (char)Rpc::InGameBuzz);
      append(&out, resp);
      out.send(peer);
    } break;
    case Rpc::InGameChoosePassOrPlay: {
      InGameChoosePassOrPlayMessage req;
      Empty resp;
      read(&in, &req);
      HandleInGameChoosePassOrPlay(client_id, &req, &resp);
      append(&out, (char)Rpc::InGameChoosePassOrPlay);
      append(&out, resp);
      out.send(peer);
    } break;
    default:
      assert(false);
  }
}

void RpcServer::GameStarted(Peer *peer, GameStartedMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::GameStarted);
  append(&out, req);
  out.send(peer);
}

void RpcServer::PlayerLeft(Peer *peer, PlayerLeftMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::PlayerLeft);
  append(&out, req);
  out.send(peer);
}

void RpcServer::GameStatePing(Peer *peer, GameStatePingMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::GameStatePing);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameStartRound(Peer *peer, InGameStartRoundMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameStartRound);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameStartFaceoff(Peer *peer, InGameStartFaceoffMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameStartFaceoff);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameAskQuestion(Peer *peer, InGameAskQuestionMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameAskQuestion);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGamePromptPassOrPlay(Peer *peer, Empty req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGamePromptPassOrPlay);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGamePlayerBuzzed(Peer *peer, InGamePlayerBuzzedMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGamePlayerBuzzed);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGamePrepForPromptForAnswer(Peer *peer, InGamePrepForPromptForAnswerMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGamePrepForPromptForAnswer);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGamePromptForAnswer(Peer *peer, InGamePromptForAnswerMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGamePromptForAnswer);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameStartPlay(Peer *peer, InGameStartPlayMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameStartPlay);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameStartSteal(Peer *peer, InGameStartStealMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameStartSteal);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGamePlayerChosePassOrPlay(Peer *peer, InGameChoosePassOrPlayMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGamePlayerChosePassOrPlay);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGamePlayerAnswered(Peer *peer, InGameAnswerMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGamePlayerAnswered);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameFlipAnswer(Peer *peer, InGameFlipAnswerMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameFlipAnswer);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameEggghhhh(Peer *peer, InGameEggghhhhMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameEggghhhh);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameEndRound(Peer *peer, InGameEndRoundMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameEndRound);
  append(&out, req);
  out.send(peer);
}

void RpcServer::InGameEndGame(Peer *peer, InGameEndGameMessage req)
{
  MessageBuilder out;
  append(&out, (char)Rpc::InGameEndGame);
  append(&out, req);
  out.send(peer);
}
