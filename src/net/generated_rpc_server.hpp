#pragma once
#include "base_rpc.hpp"
#include "generated_messages.hpp"

struct RpcServer : public BaseRpcServer {
	using BaseRpcServer::BaseRpcServer;

	void handle_rpc(ClientId, Peer*, char*, int);
	void ListGames(ClientId client_id, ListGamesRequest*, ListGamesResponse*);
	void GetGame(ClientId client_id, GetGameRequest*, GetGameResponse*);
	void CreateGame(ClientId client_id, CreateGameRequest*, CreateGameResponse*);
	void JoinGame(ClientId client_id, JoinGameRequest*, JoinGameResponse*);
	void SwapTeam(ClientId client_id, SwapTeamRequest*, SwapTeamResponse*);
	void LeaveGame(ClientId client_id, LeaveGameRequest*, LeaveGameResponse*);
	void StartGame(ClientId client_id, StartGameRequest*, StartGameResponse*);
};
void RpcServer::handle_rpc(ClientId client_id, Peer *peer, char *data, int msg_len) {
	Rpc rpc_type;
	data = read_byte(data, (char *)&rpc_type);
	MessageReader in(data, msg_len - 1);

	MessageBuilder out;
	switch(rpc_type) {
		case Rpc::ListGames: {
			ListGamesRequest req;
			ListGamesResponse resp;
			read(&in, &req);
			ListGames(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
		case Rpc::GetGame: {
			GetGameRequest req;
			GetGameResponse resp;
			read(&in, &req);
			GetGame(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
		case Rpc::CreateGame: {
			CreateGameRequest req;
			CreateGameResponse resp;
			read(&in, &req);
			CreateGame(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
		case Rpc::JoinGame: {
			JoinGameRequest req;
			JoinGameResponse resp;
			read(&in, &req);
			JoinGame(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
		case Rpc::SwapTeam: {
			SwapTeamRequest req;
			SwapTeamResponse resp;
			read(&in, &req);
			SwapTeam(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
		case Rpc::LeaveGame: {
			LeaveGameRequest req;
			LeaveGameResponse resp;
			read(&in, &req);
			LeaveGame(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
		case Rpc::StartGame: {
			StartGameRequest req;
			StartGameResponse resp;
			read(&in, &req);
			StartGame(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
	}
}

