
#pragma once
#include "net_windows.hpp"
#include <vector>

enum struct Message : char
{
	Empty = 1,
	GameMetadata,
	ListGamesRequest,
	ListGamesResponse,
	Player,
	GetGameRequest,
	GetGameResponse,
	CreateGameRequest,
	CreateGameResponse,
	JoinGameRequest,
	JoinGameResponse,
	SwapTeamRequest,
	LeaveGameRequest,
	LeaveGameResponse,
	StartGameRequest,
	StartGameResponse,
	GameStartedMessage,
	PlayerLeftMessage,
	InGameAnswerMessage,
	InGameChoosePassOrPlayMessage,
	InGameStartRoundMessage,
	GameStatePingMessage,
	InGameStartFaceoffMessage,
	InGameAskQuestionMessage,
	InGamePlayerBuzzedMessage,
	InGamePrepForPromptForAnswerMessage,
	InGamePromptForAnswerMessage,
	InGameStartPlayMessage,
	InGameStartStealMessage,
	InGameFlipAnswerMessage,
	InGameEggghhhhMessage,
	InGameEndRoundMessage
};


struct Empty 
{

};
void append(MessageBuilder *msg, Empty &in)
{

}
uint32_t read(MessageReader *msg, Empty *out)
{
    uint32_t len = 0;


    return len;
}
void append(MessageBuilder *msg, std::vector<Empty> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<Empty> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		Empty elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct GameMetadata 
{
    int32_t id = {};
    AllocatedString<64> name = {};
    AllocatedString<64> owner = {};
    bool is_self_hosted = false;
    int32_t num_players = {};
};
void append(MessageBuilder *msg, GameMetadata &in)
{
    append(msg, in.id);
    append(msg, in.name);
    append(msg, in.owner);
    append(msg, in.is_self_hosted);
    append(msg, in.num_players);
}
uint32_t read(MessageReader *msg, GameMetadata *out)
{
    uint32_t len = 0;

    len += read(msg, &out->id);
    len += read(msg, &out->name);
    len += read(msg, &out->owner);
    len += read(msg, &out->is_self_hosted);
    len += read(msg, &out->num_players);
    return len;
}
void append(MessageBuilder *msg, std::vector<GameMetadata> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<GameMetadata> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		GameMetadata elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct ListGamesRequest 
{

};
void append(MessageBuilder *msg, ListGamesRequest &in)
{

}
uint32_t read(MessageReader *msg, ListGamesRequest *out)
{
    uint32_t len = 0;


    return len;
}
void append(MessageBuilder *msg, std::vector<ListGamesRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<ListGamesRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		ListGamesRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct ListGamesResponse 
{
    std::vector<GameMetadata> games = {};
};
void append(MessageBuilder *msg, ListGamesResponse &in)
{
    append(msg, in.games);
}
uint32_t read(MessageReader *msg, ListGamesResponse *out)
{
    uint32_t len = 0;

    len += read(msg, &out->games);
    return len;
}
void append(MessageBuilder *msg, std::vector<ListGamesResponse> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<ListGamesResponse> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		ListGamesResponse elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct Player 
{
    int32_t user_id = {};
    AllocatedString<64> name = {};
    int32_t family = {};
};
void append(MessageBuilder *msg, Player &in)
{
    append(msg, in.user_id);
    append(msg, in.name);
    append(msg, in.family);
}
uint32_t read(MessageReader *msg, Player *out)
{
    uint32_t len = 0;

    len += read(msg, &out->user_id);
    len += read(msg, &out->name);
    len += read(msg, &out->family);
    return len;
}
void append(MessageBuilder *msg, std::vector<Player> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<Player> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		Player elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct GetGameRequest 
{
    int32_t game_id = {};
};
void append(MessageBuilder *msg, GetGameRequest &in)
{
    append(msg, in.game_id);
}
uint32_t read(MessageReader *msg, GetGameRequest *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<GetGameRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<GetGameRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		GetGameRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct GetGameResponse 
{
    GameMetadata game = {};
    std::vector<Player> players = {};
};
void append(MessageBuilder *msg, GetGameResponse &in)
{
    append(msg, in.game);
    append(msg, in.players);
}
uint32_t read(MessageReader *msg, GetGameResponse *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game);
    len += read(msg, &out->players);
    return len;
}
void append(MessageBuilder *msg, std::vector<GetGameResponse> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<GetGameResponse> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		GetGameResponse elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct CreateGameRequest 
{
    AllocatedString<64> name = {};
    AllocatedString<64> owner_name = {};
    bool is_self_hosted = false;
};
void append(MessageBuilder *msg, CreateGameRequest &in)
{
    append(msg, in.name);
    append(msg, in.owner_name);
    append(msg, in.is_self_hosted);
}
uint32_t read(MessageReader *msg, CreateGameRequest *out)
{
    uint32_t len = 0;

    len += read(msg, &out->name);
    len += read(msg, &out->owner_name);
    len += read(msg, &out->is_self_hosted);
    return len;
}
void append(MessageBuilder *msg, std::vector<CreateGameRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<CreateGameRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		CreateGameRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct CreateGameResponse 
{
    int32_t game_id = {};
    int32_t owner_id = {};
};
void append(MessageBuilder *msg, CreateGameResponse &in)
{
    append(msg, in.game_id);
    append(msg, in.owner_id);
}
uint32_t read(MessageReader *msg, CreateGameResponse *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game_id);
    len += read(msg, &out->owner_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<CreateGameResponse> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<CreateGameResponse> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		CreateGameResponse elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct JoinGameRequest 
{
    int32_t game_id = {};
    AllocatedString<64> player_name = {};
};
void append(MessageBuilder *msg, JoinGameRequest &in)
{
    append(msg, in.game_id);
    append(msg, in.player_name);
}
uint32_t read(MessageReader *msg, JoinGameRequest *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game_id);
    len += read(msg, &out->player_name);
    return len;
}
void append(MessageBuilder *msg, std::vector<JoinGameRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<JoinGameRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		JoinGameRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct JoinGameResponse 
{

};
void append(MessageBuilder *msg, JoinGameResponse &in)
{

}
uint32_t read(MessageReader *msg, JoinGameResponse *out)
{
    uint32_t len = 0;


    return len;
}
void append(MessageBuilder *msg, std::vector<JoinGameResponse> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<JoinGameResponse> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		JoinGameResponse elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct SwapTeamRequest 
{
    int32_t game_id = {};
    int32_t user_id = {};
};
void append(MessageBuilder *msg, SwapTeamRequest &in)
{
    append(msg, in.game_id);
    append(msg, in.user_id);
}
uint32_t read(MessageReader *msg, SwapTeamRequest *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game_id);
    len += read(msg, &out->user_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<SwapTeamRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<SwapTeamRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		SwapTeamRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct LeaveGameRequest 
{

};
void append(MessageBuilder *msg, LeaveGameRequest &in)
{

}
uint32_t read(MessageReader *msg, LeaveGameRequest *out)
{
    uint32_t len = 0;


    return len;
}
void append(MessageBuilder *msg, std::vector<LeaveGameRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<LeaveGameRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		LeaveGameRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct LeaveGameResponse 
{

};
void append(MessageBuilder *msg, LeaveGameResponse &in)
{

}
uint32_t read(MessageReader *msg, LeaveGameResponse *out)
{
    uint32_t len = 0;


    return len;
}
void append(MessageBuilder *msg, std::vector<LeaveGameResponse> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<LeaveGameResponse> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		LeaveGameResponse elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct StartGameRequest 
{
    int32_t game_id = {};
};
void append(MessageBuilder *msg, StartGameRequest &in)
{
    append(msg, in.game_id);
}
uint32_t read(MessageReader *msg, StartGameRequest *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<StartGameRequest> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<StartGameRequest> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		StartGameRequest elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct StartGameResponse 
{

};
void append(MessageBuilder *msg, StartGameResponse &in)
{

}
uint32_t read(MessageReader *msg, StartGameResponse *out)
{
    uint32_t len = 0;


    return len;
}
void append(MessageBuilder *msg, std::vector<StartGameResponse> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<StartGameResponse> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		StartGameResponse elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct GameStartedMessage 
{
    int32_t game_id = {};
    int32_t your_id = {};
};
void append(MessageBuilder *msg, GameStartedMessage &in)
{
    append(msg, in.game_id);
    append(msg, in.your_id);
}
uint32_t read(MessageReader *msg, GameStartedMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->game_id);
    len += read(msg, &out->your_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<GameStartedMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<GameStartedMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		GameStartedMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct PlayerLeftMessage 
{
    int32_t user_id = {};
};
void append(MessageBuilder *msg, PlayerLeftMessage &in)
{
    append(msg, in.user_id);
}
uint32_t read(MessageReader *msg, PlayerLeftMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->user_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<PlayerLeftMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<PlayerLeftMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		PlayerLeftMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameAnswerMessage 
{
    AllocatedString<64> answer = {};
};
void append(MessageBuilder *msg, InGameAnswerMessage &in)
{
    append(msg, in.answer);
}
uint32_t read(MessageReader *msg, InGameAnswerMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->answer);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameAnswerMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameAnswerMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameAnswerMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameChoosePassOrPlayMessage 
{
    bool play = false;
};
void append(MessageBuilder *msg, InGameChoosePassOrPlayMessage &in)
{
    append(msg, in.play);
}
uint32_t read(MessageReader *msg, InGameChoosePassOrPlayMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->play);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameChoosePassOrPlayMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameChoosePassOrPlayMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameChoosePassOrPlayMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameStartRoundMessage 
{
    int32_t round = {};
};
void append(MessageBuilder *msg, InGameStartRoundMessage &in)
{
    append(msg, in.round);
}
uint32_t read(MessageReader *msg, InGameStartRoundMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->round);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameStartRoundMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameStartRoundMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameStartRoundMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct GameStatePingMessage 
{
    int32_t my_id = {};
    std::vector<Player> players = {};
};
void append(MessageBuilder *msg, GameStatePingMessage &in)
{
    append(msg, in.my_id);
    append(msg, in.players);
}
uint32_t read(MessageReader *msg, GameStatePingMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->my_id);
    len += read(msg, &out->players);
    return len;
}
void append(MessageBuilder *msg, std::vector<GameStatePingMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<GameStatePingMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		GameStatePingMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameStartFaceoffMessage 
{
    int32_t faceoffer_0_id = {};
    int32_t faceoffer_1_id = {};
};
void append(MessageBuilder *msg, InGameStartFaceoffMessage &in)
{
    append(msg, in.faceoffer_0_id);
    append(msg, in.faceoffer_1_id);
}
uint32_t read(MessageReader *msg, InGameStartFaceoffMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->faceoffer_0_id);
    len += read(msg, &out->faceoffer_1_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameStartFaceoffMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameStartFaceoffMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameStartFaceoffMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameAskQuestionMessage 
{
    AllocatedString<64> question = {};
    int32_t num_answers = {};
};
void append(MessageBuilder *msg, InGameAskQuestionMessage &in)
{
    append(msg, in.question);
    append(msg, in.num_answers);
}
uint32_t read(MessageReader *msg, InGameAskQuestionMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->question);
    len += read(msg, &out->num_answers);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameAskQuestionMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameAskQuestionMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameAskQuestionMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGamePlayerBuzzedMessage 
{
    int32_t user_id = {};
    int32_t buzzing_family = {};
};
void append(MessageBuilder *msg, InGamePlayerBuzzedMessage &in)
{
    append(msg, in.user_id);
    append(msg, in.buzzing_family);
}
uint32_t read(MessageReader *msg, InGamePlayerBuzzedMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->user_id);
    len += read(msg, &out->buzzing_family);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGamePlayerBuzzedMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGamePlayerBuzzedMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGamePlayerBuzzedMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGamePrepForPromptForAnswerMessage 
{
    int32_t family = {};
    int32_t player_position = {};
};
void append(MessageBuilder *msg, InGamePrepForPromptForAnswerMessage &in)
{
    append(msg, in.family);
    append(msg, in.player_position);
}
uint32_t read(MessageReader *msg, InGamePrepForPromptForAnswerMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->family);
    len += read(msg, &out->player_position);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGamePrepForPromptForAnswerMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGamePrepForPromptForAnswerMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGamePrepForPromptForAnswerMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGamePromptForAnswerMessage 
{
    int32_t user_id = {};
};
void append(MessageBuilder *msg, InGamePromptForAnswerMessage &in)
{
    append(msg, in.user_id);
}
uint32_t read(MessageReader *msg, InGamePromptForAnswerMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->user_id);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGamePromptForAnswerMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGamePromptForAnswerMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGamePromptForAnswerMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameStartPlayMessage 
{
    int32_t family = {};
};
void append(MessageBuilder *msg, InGameStartPlayMessage &in)
{
    append(msg, in.family);
}
uint32_t read(MessageReader *msg, InGameStartPlayMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->family);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameStartPlayMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameStartPlayMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameStartPlayMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameStartStealMessage 
{
    int32_t family = {};
};
void append(MessageBuilder *msg, InGameStartStealMessage &in)
{
    append(msg, in.family);
}
uint32_t read(MessageReader *msg, InGameStartStealMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->family);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameStartStealMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameStartStealMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameStartStealMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameFlipAnswerMessage 
{
    int32_t answer_index = {};
    AllocatedString<64> answer = {};
    int32_t score = {};
};
void append(MessageBuilder *msg, InGameFlipAnswerMessage &in)
{
    append(msg, in.answer_index);
    append(msg, in.answer);
    append(msg, in.score);
}
uint32_t read(MessageReader *msg, InGameFlipAnswerMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->answer_index);
    len += read(msg, &out->answer);
    len += read(msg, &out->score);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameFlipAnswerMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameFlipAnswerMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameFlipAnswerMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameEggghhhhMessage 
{
    int32_t n_incorrects = {};
};
void append(MessageBuilder *msg, InGameEggghhhhMessage &in)
{
    append(msg, in.n_incorrects);
}
uint32_t read(MessageReader *msg, InGameEggghhhhMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->n_incorrects);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameEggghhhhMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameEggghhhhMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameEggghhhhMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};

struct InGameEndRoundMessage 
{
    int32_t round_winner = {};
    int32_t family0_score = {};
    int32_t family1_score = {};
};
void append(MessageBuilder *msg, InGameEndRoundMessage &in)
{
    append(msg, in.round_winner);
    append(msg, in.family0_score);
    append(msg, in.family1_score);
}
uint32_t read(MessageReader *msg, InGameEndRoundMessage *out)
{
    uint32_t len = 0;

    len += read(msg, &out->round_winner);
    len += read(msg, &out->family0_score);
    len += read(msg, &out->family1_score);
    return len;
}
void append(MessageBuilder *msg, std::vector<InGameEndRoundMessage> &in)
{
	append(msg, (uint16_t)in.size());
	for (auto &it : in)
    {
        append(msg, it);
    }
};
uint32_t read(MessageReader *msg, std::vector<InGameEndRoundMessage> *out)
{
	uint32_t len = 0;

	uint16_t list_len;
	len += read(msg, &list_len);
	for (int i = 0; i < list_len; i++)
    {
		InGameEndRoundMessage elem;
        len += read(msg, &elem);
        out->push_back(elem);
	}

	return len;
};


enum struct Rpc : char
{
	ListGames = 1,
	GetGame,
	CreateGame,
	JoinGame,
	SwapTeam,
	LeaveGame,
	StartGame,
	InGameReady,
	InGameAnswer,
	InGameBuzz,
	InGameChoosePassOrPlay,
	GameStarted,
	PlayerLeft,
	GameStatePing,
	InGameStartRound,
	InGameStartFaceoff,
	InGameAskQuestion,
	InGamePromptPassOrPlay,
	InGamePlayerBuzzed,
	InGamePrepForPromptForAnswer,
	InGamePromptForAnswer,
	InGameStartPlay,
	InGameStartSteal,
	InGamePlayerChosePassOrPlay,
	InGamePlayerAnswered,
	InGameFlipAnswer,
	InGameEggghhhh,
	InGameEndRound,
	InGameEndGame
};
