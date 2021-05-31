#pragma once
#include "generated_messages.hpp"

struct RpcClient {
	Peer peer;
	RpcClient(const char *address, uint16_t port) {
		peer.open(address, port, true);
	}

	void ListGames(ListGamesRequest &, ListGamesResponse*);
	ListGamesResponse ListGames(ListGamesRequest);

	void GetGame(GetGameRequest &, GetGameResponse*);
	GetGameResponse GetGame(GetGameRequest);

	void CreateGame(CreateGameRequest &, CreateGameResponse*);
	CreateGameResponse CreateGame(CreateGameRequest);

	void JoinGame(JoinGameRequest &, JoinGameResponse*);
	JoinGameResponse JoinGame(JoinGameRequest);

	void LeaveGame(LeaveGameRequest &, LeaveGameResponse*);
	LeaveGameResponse LeaveGame(LeaveGameRequest);

};

void RpcClient::ListGames(ListGamesRequest &req, ListGamesResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::ListGames); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) < 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}
ListGamesResponse RpcClient::ListGames(ListGamesRequest req) {
	ListGamesResponse resp;
	ListGames(req, &resp);
	return resp;
}

void RpcClient::GetGame(GetGameRequest &req, GetGameResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::GetGame); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) < 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}
GetGameResponse RpcClient::GetGame(GetGameRequest req) {
	GetGameResponse resp;
	GetGame(req, &resp);
	return resp;
}

void RpcClient::CreateGame(CreateGameRequest &req, CreateGameResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::CreateGame); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) < 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}
CreateGameResponse RpcClient::CreateGame(CreateGameRequest req) {
	CreateGameResponse resp;
	CreateGame(req, &resp);
	return resp;
}

void RpcClient::JoinGame(JoinGameRequest &req, JoinGameResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::JoinGame); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) < 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}
JoinGameResponse RpcClient::JoinGame(JoinGameRequest req) {
	JoinGameResponse resp;
	JoinGame(req, &resp);
	return resp;
}

void RpcClient::LeaveGame(LeaveGameRequest &req, LeaveGameResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::LeaveGame); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) < 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}
LeaveGameResponse RpcClient::LeaveGame(LeaveGameRequest req) {
	LeaveGameResponse resp;
	LeaveGame(req, &resp);
	return resp;
}


