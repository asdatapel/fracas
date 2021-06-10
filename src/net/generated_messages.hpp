#include "net_windows.hpp"
#include <vector>

struct GameMetadata {
	uint32_t id = {}; 
	AllocatedString<64> name = {}; 
	AllocatedString<64> owner = {}; 
	bool is_self_hosted = false; 
	int32_t num_players = {}; 
};
void append(MessageBuilder *msg, GameMetadata &in) {
	append(msg, in.id);
	append(msg, in.name);
	append(msg, in.owner);
	append(msg, in.is_self_hosted);
	append(msg, in.num_players);
};
uint32_t read(MessageReader *msg, GameMetadata *out) {
	uint32_t len = 0;

	len += read(msg, &out->id);
	len += read(msg, &out->name);
	len += read(msg, &out->owner);
	len += read(msg, &out->is_self_hosted);
	len += read(msg, &out->num_players);

	return len;
};
void append(MessageBuilder *msg, std::vector<GameMetadata> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<GameMetadata> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		GameMetadata elem; len += read(msg, &elem); out->push_back(elem);
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
	std::vector<GameMetadata> games = {}; 
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

struct Player {
	uint32_t user_id = {}; 
	AllocatedString<64> name = {}; 
	bool team = false; 
};
void append(MessageBuilder *msg, Player &in) {
	append(msg, in.user_id);
	append(msg, in.name);
	append(msg, in.team);
};
uint32_t read(MessageReader *msg, Player *out) {
	uint32_t len = 0;

	len += read(msg, &out->user_id);
	len += read(msg, &out->name);
	len += read(msg, &out->team);

	return len;
};
void append(MessageBuilder *msg, std::vector<Player> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<Player> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		Player elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct GetGameRequest {
	uint32_t game_id = {}; 
};
void append(MessageBuilder *msg, GetGameRequest &in) {
	append(msg, in.game_id);
};
uint32_t read(MessageReader *msg, GetGameRequest *out) {
	uint32_t len = 0;

	len += read(msg, &out->game_id);

	return len;
};
void append(MessageBuilder *msg, std::vector<GetGameRequest> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<GetGameRequest> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		GetGameRequest elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct GetGameResponse {
	GameMetadata game = {}; 
	std::vector<Player> players = {}; 
};
void append(MessageBuilder *msg, GetGameResponse &in) {
	append(msg, in.game);
	append(msg, in.players);
};
uint32_t read(MessageReader *msg, GetGameResponse *out) {
	uint32_t len = 0;

	len += read(msg, &out->game);
	len += read(msg, &out->players);

	return len;
};
void append(MessageBuilder *msg, std::vector<GetGameResponse> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<GetGameResponse> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		GetGameResponse elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct CreateGameRequest {
	AllocatedString<64> name = {}; 
	AllocatedString<64> owner_name = {}; 
	bool is_self_hosted = false; 
};
void append(MessageBuilder *msg, CreateGameRequest &in) {
	append(msg, in.name);
	append(msg, in.owner_name);
	append(msg, in.is_self_hosted);
};
uint32_t read(MessageReader *msg, CreateGameRequest *out) {
	uint32_t len = 0;

	len += read(msg, &out->name);
	len += read(msg, &out->owner_name);
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
	uint32_t game_id = {}; 
	uint32_t owner_id = {}; 
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

struct JoinGameRequest {
	uint32_t game_id = {}; 
	AllocatedString<64> player_name = {}; 
};
void append(MessageBuilder *msg, JoinGameRequest &in) {
	append(msg, in.game_id);
	append(msg, in.player_name);
};
uint32_t read(MessageReader *msg, JoinGameRequest *out) {
	uint32_t len = 0;

	len += read(msg, &out->game_id);
	len += read(msg, &out->player_name);

	return len;
};
void append(MessageBuilder *msg, std::vector<JoinGameRequest> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<JoinGameRequest> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		JoinGameRequest elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct JoinGameResponse {
};
void append(MessageBuilder *msg, JoinGameResponse &in) {
};
uint32_t read(MessageReader *msg, JoinGameResponse *out) {
	uint32_t len = 0;


	return len;
};
void append(MessageBuilder *msg, std::vector<JoinGameResponse> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<JoinGameResponse> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		JoinGameResponse elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct SwapTeamRequest {
	uint32_t game_id = {}; 
	uint32_t user_id = {}; 
};
void append(MessageBuilder *msg, SwapTeamRequest &in) {
	append(msg, in.game_id);
	append(msg, in.user_id);
};
uint32_t read(MessageReader *msg, SwapTeamRequest *out) {
	uint32_t len = 0;

	len += read(msg, &out->game_id);
	len += read(msg, &out->user_id);

	return len;
};
void append(MessageBuilder *msg, std::vector<SwapTeamRequest> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<SwapTeamRequest> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		SwapTeamRequest elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct SwapTeamResponse {
};
void append(MessageBuilder *msg, SwapTeamResponse &in) {
};
uint32_t read(MessageReader *msg, SwapTeamResponse *out) {
	uint32_t len = 0;


	return len;
};
void append(MessageBuilder *msg, std::vector<SwapTeamResponse> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<SwapTeamResponse> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		SwapTeamResponse elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct LeaveGameRequest {
};
void append(MessageBuilder *msg, LeaveGameRequest &in) {
};
uint32_t read(MessageReader *msg, LeaveGameRequest *out) {
	uint32_t len = 0;


	return len;
};
void append(MessageBuilder *msg, std::vector<LeaveGameRequest> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<LeaveGameRequest> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		LeaveGameRequest elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

struct LeaveGameResponse {
};
void append(MessageBuilder *msg, LeaveGameResponse &in) {
};
uint32_t read(MessageReader *msg, LeaveGameResponse *out) {
	uint32_t len = 0;


	return len;
};
void append(MessageBuilder *msg, std::vector<LeaveGameResponse> &in) {
	append(msg, (uint16_t)in.size());
	for (auto &it : in){append(msg, it);}
};
uint32_t read(MessageReader *msg, std::vector<LeaveGameResponse> *out) {
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++) {
		LeaveGameResponse elem; len += read(msg, &elem); out->push_back(elem);
	}

	return len;
};

enum struct Rpc : char {
	ListGames = 1,
	GetGame,
	CreateGame,
	JoinGame,
	SwapTeam,
	LeaveGame,
};

