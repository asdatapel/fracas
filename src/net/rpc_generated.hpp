#include "net_windows.hpp"
#include <vector>

struct Game {
	uint32_t id; 
	String name; 
	String owner; 
	bool is_self_hosted; 
	int32_t num_players; 
};
void append(MessageBuilder *msg, Game &in) {
	append(msg, in.id);
	append(msg, in.name);
	append(msg, in.owner);
	append(msg, in.is_self_hosted);
	append(msg, in.num_players);
};
uint32_t read(MessageReader *msg, Game *out) {
	uint32_t len = 0;

	len += read(msg, &out->id);
	len += read(msg, &out->name);
	len += read(msg, &out->owner);
	len += read(msg, &out->is_self_hosted);
	len += read(msg, &out->num_players);

	return len;
};
void append(MessageBuilder *msg, std::vector<Game> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<Game> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		Game elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct ListGamesRequest {
};
void append(MessageBuilder *msg, ListGamesRequest &in) {
};
uint32_t read(MessageReader *msg, ListGamesRequest *out) {
	uint32_t len = 0;


	return len;
};
void append(MessageBuilder *msg, std::vector<ListGamesRequest> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<ListGamesRequest> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		ListGamesRequest elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct ListGamesResponse {
	std::vector<Game> games; 
};
void append(MessageBuilder *msg, ListGamesResponse &in) {
	append(msg, in.games);
};
uint32_t read(MessageReader *msg, ListGamesResponse *out) {
	uint32_t len = 0;

	len += read(msg, &out->games);

	return len;
};
void append(MessageBuilder *msg, std::vector<ListGamesResponse> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<ListGamesResponse> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		ListGamesResponse elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct CreateGameRequest {
	String name; 
	bool is_self_hosted; 
};
void append(MessageBuilder *msg, CreateGameRequest &in) {
	append(msg, in.name);
	append(msg, in.is_self_hosted);
};
uint32_t read(MessageReader *msg, CreateGameRequest *out) {
	uint32_t len = 0;

	len += read(msg, &out->name);
	len += read(msg, &out->is_self_hosted);

	return len;
};
void append(MessageBuilder *msg, std::vector<CreateGameRequest> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<CreateGameRequest> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		CreateGameRequest elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct CreateGameResponse {
	uint32_t game_id; 
	uint32_t owner_id; 
};
void append(MessageBuilder *msg, CreateGameResponse &in) {
	append(msg, in.game_id);
	append(msg, in.owner_id);
};
uint32_t read(MessageReader *msg, CreateGameResponse *out) {
	uint32_t len = 0;

	len += read(msg, &out->game_id);
	len += read(msg, &out->owner_id);

	return len;
};
void append(MessageBuilder *msg, std::vector<CreateGameResponse> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<CreateGameResponse> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		CreateGameResponse elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

enum struct Rpc : char {
	ListGames = 1,
	CreateGame,
};

struct ServerData;
struct RpcServer {
	ServerData *server_data;

	void handle_rpc(ClientId, Peer*, char*, int);
	void ListGames(ClientId client_id, ListGamesRequest*, ListGamesResponse*);
	void CreateGame(ClientId client_id, CreateGameRequest*, CreateGameResponse*);
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
		case Rpc::CreateGame: {
			CreateGameRequest req;
			CreateGameResponse resp;
			read(&in, &req);
			CreateGame(client_id, &req, &resp);
			append(&out, resp);
			out.send(peer);
		} break;
	}
}

struct RpcClient {
	Peer peer;
	RpcClient(const char *address, uint16_t port) {
		peer.open(address, port, true);
	}

	void ListGames(ListGamesRequest &, ListGamesResponse*);
	void CreateGame(CreateGameRequest &, CreateGameResponse*);
};

void RpcClient::ListGames(ListGamesRequest &req, ListGamesResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::ListGames); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) <= 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}

void RpcClient::CreateGame(CreateGameRequest &req, CreateGameResponse *resp) {
	MessageBuilder out; append(&out, (char) Rpc::CreateGame); append(&out, req); out.send(&peer);

	int msg_len; char msg[MAX_MSG_SIZE];while ((msg_len = peer.recieve_msg(msg)) <= 0){}
	MessageReader in(msg, msg_len); read(&in, resp);
}


