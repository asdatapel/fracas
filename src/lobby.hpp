#pragma once

#include <unordered_map>

#include "common.hpp"
#include "game_state.hpp"
#include "net/net.hpp"
#include "net/generated_rpc_server.hpp"

void error(String msg) {
    printf("%.*s\n", msg.len, msg.data);
}

const uint64_t second = 1000000000;

struct ServerData
{
    std::unordered_map<GameId, Lobby> lobbies;
    std::unordered_map<ClientId, Client> clients;
};

struct GameProperties
{
    ClientId owner;
    AllocatedString<64> name = {};
    bool is_self_hosted = false;
    AllocatedString<64> owner_name;
};
enum struct LobbyStage
{
    NOT_STARTED,
    IN_GAME,
    ENDED,
    DEAD,
};
struct Lobby
{
    GameProperties properties;
    LobbyStage stage = LobbyStage::NOT_STARTED;
    GameState game = {};

    typedef void (Lobby::*Stage)(Broadcaster);
    typedef void (Lobby::*Waiter)(Broadcaster, uint64_t);
    Stage next_stage = nullptr;
    Waiter waiter = nullptr;
    uint64_t waiter_deadline = 0;
    uint64_t waiter_elapsed = 0;

    u64 ping_elapsed = 0;

    bool ready_to_delete = false;

    Lobby() = default;
    Lobby(GameProperties properties)
    {
        this->properties = properties;
    }

    void set_waiter(Waiter waiter, uint64_t deadline)
    {
        this->waiter = waiter;
        waiter_deadline = deadline;
        waiter_elapsed = 0;
    }

    void set_next_stage(Stage stage)
    {
        for (int i = 0; i < game.players.len; i++)
        {
            game.players[i].ready = false;
        }
        next_stage = stage;

        set_waiter(&Lobby::waiter_all_ready, 30 * second);
    }

    void add_player(ClientId client_id, AllocatedString<64> name)
    {
        if (game.is_player_in_this_game(client_id))
        {
            // TODO maybe return error
            error("player is already in this game");
            return;
        }
        if (game.num_players() >= MAX_PLAYERS_PER_GAME)
        {
            // TODO return error GAME_FULL
            error("game is full");
            return;
        }

        int next_family = 0;
        if (game.num_players(1) < game.num_players(0))
        {
            next_family = 1;
        }
        game.players.append({client_id, name, next_family});
    }

    void remove_player(ClientId client_id, Broadcaster broadcaster)
    {
        for (int i = 0; i < game.players.len; i++)
        {
            if (game.players[i].id == client_id)
            {
                game.players.shift_delete(i);
            }
        }

        if (properties.owner == client_id)
        {
            end_game(broadcaster);
            // MAYBETODO we can switch the owner instead, as long as the game isn't self hosted
        }

        if (stage != LobbyStage::NOT_STARTED && (game.num_players(0) == 0 || game.num_players(1) == 0))
        {
            end_game(broadcaster);
        }

        broadcaster.broadcast(&RpcServer::PlayerLeft, PlayerLeftMessage{client_id});
    }

    bool ready_to_start()
    {
        if (stage != LobbyStage::NOT_STARTED)
            return false;

        int fam_0_count = game.num_players(0);
        int fam_1_count = game.num_players(1);
        return fam_0_count > 0 && fam_0_count < MAX_PLAYERS_PER_GAME / 2 &&
               fam_1_count > 0 && fam_1_count < MAX_PLAYERS_PER_GAME / 2;
    }

    void start_game(Broadcaster broadcaster, GameId game_id)
    {
        // game = GameState(); TODO this resets players which are now stored in GameState. Either remove them from GameState or make a GameState::reset function
        stage = LobbyStage::IN_GAME;

        for (int i = 0; i < game.players.len; i++)
        {
            Peer *peer = &broadcaster.rpc_server->server_data->clients.at(game.players[i].id).peer;
            ((RpcServer *)broadcaster.rpc_server)->GameStarted(peer, GameStartedMessage{game_id, game.players[i].id});
        }

        set_next_stage(&Lobby::stage_start_round);
    }

    bool has_started()
    {
        return game.round >= 0;
    }

    void end_game(Broadcaster broadcaster)
    {
        stage = LobbyStage::ENDED;
        game.round_stage = RoundStage::END;

        InGameEndGameMessage msg;
        msg.game_winner = game.scores[0] > game.scores[1] ? 0 : 1;
        broadcaster.broadcast(&RpcServer::InGameEndGame, msg);

        set_waiter(&Lobby::waiter_end_game, 10 * 60 * second);
    }

    void do_next_stage(Broadcaster broadcaster)
    {
        (this->*next_stage)(broadcaster);
    }

    void stage_start_round(Broadcaster broadcaster)
    {
        game.round++;
        game.round_stage = RoundStage::START;

        game.playing_family = -1;
        game.buzzing_family = -1;
        game.faceoff_winning_family = -1;

        game.this_round_points = 0;
        game.incorrects = 0;
        game.round_winner = -1;

        game.last_answer.len = 0;

        game.answers.clear();
        game.question = string_to_allocated_string<128>("name a color bitch bitch stupid bitch?");
        game.answers.append({false, 98, "RED"});
        game.answers.append({false, 87, "GREEN"});
        game.answers.append({false, 76, "BLUE"});
        game.answers.append({false, 65, "ORANGE"});
        game.answers.append({false, 54, "PINK"});
        game.answers.append({false, 43, "PURPLE"});
        game.answers.append({false, 32, "MUAVE"});

        broadcaster.broadcast(&RpcServer::InGameStartRound, InGameStartRoundMessage{game.round});
        set_next_stage(&Lobby::stage_start_faceoff);
    }

    void stage_start_faceoff(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::FACEOFF;

        auto faceoffers = game.faceoff_players();
        broadcaster.broadcast(&RpcServer::InGameStartFaceoff, InGameStartFaceoffMessage{faceoffers.first->id, faceoffers.second->id});
        set_next_stage(&Lobby::stage_ask_question);
    }

    void stage_ask_question(Broadcaster broadcaster)
    {
        broadcaster.broadcast(&RpcServer::InGameAskQuestion, InGameAskQuestionMessage{game.question, (int32_t) game.answers.len});
        set_waiter(&Lobby::waiter_buzz, 25 * second);
    }

    void stage_prep_for_prompt_for_answer(Broadcaster broadcaster)
    {
        auto answerer = game.who_can_answer();
        InGamePrepForPromptForAnswerMessage msg;
        msg.family = answerer->family;
        msg.player_position = game.get_player_position(game.who_can_answer()->id);
        broadcaster.broadcast(&RpcServer::InGamePrepForPromptForAnswer, msg);
        set_next_stage(&Lobby::stage_prompt_for_answer);
    }

    void stage_prompt_for_answer(Broadcaster broadcaster)
    {
        InGamePromptForAnswerMessage msg;
        msg.user_id = game.who_can_answer()->id;
        broadcaster.broadcast(&RpcServer::InGamePromptForAnswer, msg);
        set_waiter(&Lobby::waiter_answer, 15 * second); // TODO increase this
    }

    void stage_respond_to_answer(Broadcaster broadcaster)
    {
        int this_answer_incorrect = 0; // used to simplify checking which faceoffer is answering
        int answer_i = game.check_answer();
        int score = answer_i == -1 ? 0 : game.answers[answer_i].score;
        if (answer_i == -1)
        {
            this_answer_incorrect = 1;
            game.incorrects += 1;

            broadcaster.broadcast(&RpcServer::InGameEggghhhh, InGameEggghhhhMessage{game.incorrects});
        }
        else
        {
            game.answers[answer_i].revealed = true;
            game.this_round_points += score * ROUND_MULTIPLIERS[game.round];

            broadcaster.broadcast(&RpcServer::InGameFlipAnswer, InGameFlipAnswerMessage{answer_i, game.last_answer, score});
        }

        if (game.round_stage == RoundStage::FACEOFF)
        {
            if (game.this_round_points == score && game.incorrects == this_answer_incorrect) // first to answer
            {
                if (answer_i == 0)
                {
                    // buzzer wins automatically
                    game.faceoff_winning_family = game.buzzing_family;
                    set_next_stage(&Lobby::stage_prompt_pass_or_play);
                }
                else
                {
                    set_next_stage(&Lobby::stage_prep_for_prompt_for_answer);
                }
            }
            else
            {
                if (game.this_round_points == 0)
                {
                    // both faceoffers failed, move on to next round
                    set_next_stage(&Lobby::stage_end_round);
                }
                else if (score >= game.this_round_points / 2)
                {
                    // non-buzzer wins
                    game.faceoff_winning_family = 1 - game.buzzing_family;
                    set_next_stage(&Lobby::stage_prompt_pass_or_play);
                }
                else
                {
                    // buzzer wins
                    game.faceoff_winning_family = game.buzzing_family;
                    set_next_stage(&Lobby::stage_prompt_pass_or_play);
                }
            }
        }
        else if (game.round_stage == RoundStage::STEAL)
        {
            if (answer_i == -1) // wrong answer
            {
                game.round_winner = game.playing_family;
            }
            else // right answer
            {
                game.round_winner = 1 - game.playing_family;
            }

            // always progress round, since stealing family only gets one chance
            set_next_stage(&Lobby::stage_end_round);
        }
        else
        {
            if (game.incorrects == 3)
            {
                set_next_stage(&Lobby::stage_start_steal);
            }
            else
            {
                game.current_players[game.playing_family] += 1;

                if (game.are_all_answers_flipped())
                {
                    game.round_winner = game.playing_family;
                    set_next_stage(&Lobby::stage_end_round);
                }
                else
                {
                    set_next_stage(&Lobby::stage_prep_for_prompt_for_answer);
                }
            }
        }
    };

    void stage_prompt_pass_or_play(Broadcaster broadcaster)
    {
        broadcaster.broadcast(&RpcServer::InGamePromptPassOrPlay, Empty{});
        set_waiter(&Lobby::waiter_pass_or_play, 30 * second);
    }

    void stage_start_play(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::PLAY;
        game.incorrects = 0;

        broadcaster.broadcast(&RpcServer::InGameStartPlay, InGameStartPlayMessage{game.faceoff_winning_family});
        set_next_stage(&Lobby::stage_prep_for_prompt_for_answer);
    }

    void stage_start_steal(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::STEAL;
        broadcaster.broadcast(&RpcServer::InGameStartSteal, InGameStartStealMessage{1 - game.faceoff_winning_family});
        set_next_stage(&Lobby::stage_prep_for_prompt_for_answer);
    }

    void stage_end_round(Broadcaster broadcaster)
    {
        game.round_stage = RoundStage::END;

        if (game.round_winner != -1)
        {
            game.scores[game.round_winner] += game.this_round_points;
        }

        InGameEndRoundMessage msg;
        msg.round_winner = game.round_winner;
        msg.family0_score = game.scores[0];
        msg.family1_score = game.scores[1];
        broadcaster.broadcast(&RpcServer::InGameEndRound, msg);

        if (game.round < 2) {
            set_next_stage(&Lobby::stage_start_round); }
        else {
            set_next_stage(&Lobby::stage_end_game);
        }
    }

    void stage_end_game(Broadcaster broadcaster)
    {
        end_game(broadcaster);
    }

    void waiter_buzz(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            game.incorrects += 1;
            broadcaster.broadcast(&RpcServer::InGameEggghhhh, InGameEggghhhhMessage{game.incorrects});
            set_next_stage(&Lobby::stage_end_round);
        }

        printf("buzz_waiter - time_elapsed: %llu\n", waiter_elapsed);
    }
    void waiter_pass_or_play(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            // default to PLAY
            game.playing_family = game.faceoff_winning_family;
            broadcaster.broadcast(&RpcServer::InGamePlayerChosePassOrPlay, InGameChoosePassOrPlayMessage{true});
            set_next_stage(&Lobby::stage_start_play);
        }
        printf("waiter_pass_or_play - time_elapsed: %llu\n", waiter_elapsed);
    }
    void waiter_answer(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            AllocatedString<64> empty_answer = string_to_allocated_string<64>("...");
            game.last_answer = empty_answer;
            broadcaster.broadcast(&RpcServer::InGamePlayerAnswered, InGameAnswerMessage{empty_answer});

            set_next_stage(&Lobby::stage_respond_to_answer);
        }
        printf("waiter_answer - time_e lapsed: %llu\n", waiter_elapsed);
    }
    void waiter_all_ready(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            printf("waiter_all_ready - timeout\n");
            do_next_stage(broadcaster);
        }
        // printf("waiter_all_ready - time_elapsed: %llu\n", waiter_elapsed);
    }
    void waiter_end_game(Broadcaster broadcaster, uint64_t elapsed_micros)
    {
        waiter_elapsed += elapsed_micros;
        if (waiter_elapsed >= waiter_deadline)
        {
            ready_to_delete = true;
        }
        //printf("waiter_end_game - time_elapsed: %llu\n", waiter_elapsed);
    }

    void tick(Broadcaster broadcaster, uint64_t elapsed_time)
    {
        if (waiter)
        {
            (this->*waiter)(broadcaster, elapsed_time);
        }

        ping_elapsed += elapsed_time;
        if (ping_elapsed > 1 * second) {
            ping_elapsed = 0;


            GameStatePingMessage msg;
            for (int i = 0; i < game.players.len; i++) {
              msg.players.push_back(
                  {game.players[i].id, game.players[i].name, game.players[i].family == 1});
            }
            for (int i = 0; i < game.players.len; i++)
            {
                Peer *peer = &broadcaster.rpc_server->server_data->clients.at(game.players[i].id).peer;
                msg.my_id = game.players[i].id;
                ((RpcServer *)broadcaster.rpc_server)->GameStatePing(peer, msg);
            }
        }
    }
};

template <typename FN, typename MSG>
void Broadcaster::broadcast(FN fn, MSG msg)
{
    for (int i = 0; i < lobby->game.players.len; i++)
    {
        ClientId client_id = lobby->game.players[i].id;

        if (rpc_server->server_data->clients.count(client_id))
        {
            (((RpcServer *)rpc_server)->*fn)(&rpc_server->server_data->clients.at(client_id).peer, msg);
        }
    }
}

ClientId add_client(ServerData *server_data, SOCKET s)
{
    static ClientId next_client_id = 1;
    next_client_id++;

    if (server_data->clients.size() > MAX_CLIENTS)
    {
        // TODO send error message SERVER_FULL
        error("too many connections");
        return 0;
    }

    Client client;
    client.peer.s = s;
    client.client_id = next_client_id;

    server_data->clients[next_client_id] = client;
    return next_client_id;
}

GameId add_game(ServerData *server_data, GameProperties properties)
{
    static GameId next_game_id = 1;
    next_game_id++;

    if (server_data->lobbies.size() > MAX_GAMES)
    {
        return 0;
    }

    server_data->lobbies[next_game_id] = Lobby(properties);
    server_data->lobbies[next_game_id].add_player(properties.owner, properties.owner_name);
    return next_game_id;
}

template <size_t N>
AllocatedString<N> sanitize_name(AllocatedString<N> name)
{
    AllocatedString<N> ret;
    bool last_char_was_whitespace = true;
    for (int i = 0; i < name.len; i++)
    {
        if (!isspace(name.data[i]))
        {
            last_char_was_whitespace = false;
            ret.append(name.data[i]);
        }
        else if (name.data[i] == ' ' && !last_char_was_whitespace)
        {
            last_char_was_whitespace = true;
            ret.append(name.data[i]);
        }
    }

    // remove trailing space
    if (ret.len > 0 && isspace(ret.data[ret.len - 1]))
    {
        ret.len -= 1;
    }

    return ret;
}

void BaseRpcServer::on_disconnect(ClientId client_id)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id && server_data->lobbies.count(client->game_id))
    {
        Lobby *lobby = &server_data->lobbies[client->game_id];
        lobby->remove_player(client_id, {this, lobby});
    }
}

void RpcServer::HandleListGames(ClientId client_id, ListGamesRequest *req, ListGamesResponse *resp)
{

    for (auto it : server_data->lobbies)
    {
        GameId game_id = it.first;
        Lobby *lobby = &it.second;

        if (lobby->stage == LobbyStage::NOT_STARTED)
        {
            GameMetadata game_msg;
            game_msg.id = game_id;
            game_msg.name = lobby->properties.name;
            game_msg.owner = server_data->clients[lobby->properties.owner].username;
            game_msg.num_players = lobby->game.num_players();
            game_msg.is_self_hosted = lobby->properties.is_self_hosted;
            resp->games.push_back(game_msg);
        }
    }
}

void RpcServer::HandleGetGame(ClientId client_id, GetGameRequest *req, GetGameResponse *resp)
{
    if (server_data->lobbies.count(req->game_id) == 0)
    {
        // TODO send NOT_FOUND
        error("game not found");
        return;
    }

    Lobby *lobby = &server_data->lobbies[req->game_id];
    resp->game.id = req->game_id;
    resp->game.name = lobby->properties.name;
    resp->game.owner = server_data->clients[lobby->properties.owner].username;
    resp->game.num_players = lobby->game.num_players();
    resp->game.is_self_hosted = lobby->properties.is_self_hosted;
    for (int i = 0; i < lobby->game.players.len; i++)
    {
        resp->players.push_back({lobby->game.players[i].id,
                                 lobby->game.players[i].name,
                                 lobby->game.players[i].family == 1});
    }
}

void RpcServer::HandleCreateGame(ClientId client_id, CreateGameRequest *req, CreateGameResponse *resp)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id)
    {
        return; // client is already in a game
    }

    // TODO more validation on the names
    auto game_name = sanitize_name(req->name);
    auto owner_name = sanitize_name(req->owner_name);
    if (game_name.len == 0)
    {
        error("invalid game name");
        return; // TODO invalid name
    }
    if (owner_name.len == 0)
    {
        error("invalid ownder name");
        return; // TODO invalid name
    }
    if (!req->is_self_hosted)
    {
        error("invalid game name");
        return; // TODO temporary
    }

    GameProperties game_properties;
    game_properties.owner = client->client_id;
    game_properties.name = game_name;
    game_properties.is_self_hosted = req->is_self_hosted;
    game_properties.owner_name = owner_name;

    GameId game_id = add_game(server_data, game_properties);
    if (!game_id)
    {
        // TODO send error message TOO_MANY_GAMES
    }

    Lobby *lobby = &server_data->lobbies[game_id];
    resp->game_id = game_id;
    resp->owner_id = client->client_id;
    client->game_id = game_id;
}

void RpcServer::HandleJoinGame(ClientId client_id, JoinGameRequest *req, JoinGameResponse *resp)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id)
    {
        return; // client is already in a game
    }
    if (server_data->lobbies.count(req->game_id) == 0)
    {
        // TODO send error, game NOT_FOUND
        return;
    }

    // TODO more validation on the name
    auto player_name = sanitize_name(req->player_name);
    if (player_name.len == 0)
    {
        return; // TODO invalid name
    }

    Lobby *lobby = &server_data->lobbies[req->game_id];
    lobby->add_player(client_id, player_name);
    client->game_id = req->game_id;
}

void RpcServer::HandleSwapTeam(ClientId client_id, SwapTeamRequest *req, Empty *resp)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id != req->game_id)
    {
        return; // client is not in the requested game
    }
    if (server_data->lobbies.count(req->game_id) == 0)
    {
        // TODO send error, game NOT_FOUND
        return;
    }

    Lobby *lobby = &server_data->lobbies[req->game_id];
    if (lobby->properties.owner != client_id)
    {
        return; // TODO client doesn't own this game
    }
    if (lobby->stage != LobbyStage::NOT_STARTED)
    {
        return; // TODO cannot swap after game has started
    }
    if (!lobby->game.is_player_in_this_game(req->user_id))
    {
        return; // error requested user isn't in this game
    }

    for (int i = 0; i < lobby->game.players.len; i++)
    {
        if (lobby->game.players[i].id == req->user_id)
        {
            lobby->game.players[i].family = 1 - lobby->game.players[i].family;
        }
    }
}

void RpcServer::HandleLeaveGame(ClientId client_id, LeaveGameRequest *req, LeaveGameResponse *resp)
{
    printf("HandleLeaveGame - player: %i\n", client_id);

    Client *client = &server_data->clients[client_id];
    if (!client->game_id)
    {
        return; // client is not in a game
    }
    if (server_data->lobbies.count(client->game_id) == 0)
    {
        // TODO send error INTERNAL_ERROR
        return;
    }

    Lobby *lobby = &server_data->lobbies[client->game_id];
    lobby->remove_player(client->client_id, {this, lobby});
    client->game_id = 0;
}

void RpcServer::HandleStartGame(ClientId client_id, StartGameRequest *req, StartGameResponse *resp)
{
    printf("HandleStartGame - player: %i, game: %i\n", client_id, req->game_id);

    Client *client = &server_data->clients[client_id];
    if (server_data->lobbies.count(client->game_id) == 0)
    {
        // TODO send error INTERNAL_ERROR
        return;
    }
    Lobby *lobby = &server_data->lobbies[req->game_id];
    if (lobby->properties.owner != client_id)
    {
        return; // TODO client doesn't own this game
    }

    // TODO check that game is ready to start
    if (!lobby->ready_to_start())
    {
        return; // TODO error
    }

    lobby->start_game({this, lobby}, req->game_id);
}

void RpcServer::HandleInGameReady(ClientId client_id, Empty *req, Empty *resp)
{
    printf("HandleInGameReady - player: %i\n", client_id);

    Client *client = &server_data->clients[client_id];
    if (server_data->lobbies.count(client->game_id) == 0)
    {
        // TODO send error PERMISSION_DENIED
        return;
    }

    Lobby *lobby = &server_data->lobbies[client->game_id];
    if (lobby->waiter != &Lobby::waiter_all_ready)
    {
        return;
    }

    PlayerData *player_data = lobby->game.get_player_data(client_id);
    player_data->ready = true;

    if (lobby->game.are_all_players_ready())
    {
        lobby->do_next_stage({this, lobby});
    }
}

void RpcServer::HandleInGameBuzz(ClientId client_id, Empty *req, Empty *resp)
{
    printf("HandleInGameBuzz - player: %i\n", client_id);

    Client *client = &server_data->clients[client_id];
    if (server_data->lobbies.count(client->game_id) == 0)
    {
        // TODO send error PERMISSION_DENIED
        return;
    }

    Lobby *lobby = &server_data->lobbies[client->game_id];
    if (lobby->waiter != &Lobby::waiter_buzz)
    {
        return;
    }

    auto [fp0, fp1] = lobby->game.faceoff_players();
    if (client_id == fp0->id)
    {
        lobby->game.buzzing_family = 0;
    }
    else if (client_id == fp1->id)
    {
        lobby->game.buzzing_family = 1;
    }
    else
    {
        return;
    }

    Broadcaster{this, lobby}.broadcast(&RpcServer::InGamePlayerBuzzed, InGamePlayerBuzzedMessage{client_id, lobby->game.buzzing_family});
    lobby->set_next_stage(&Lobby::stage_prep_for_prompt_for_answer);
}

void RpcServer::HandleInGameAnswer(ClientId client_id, InGameAnswerMessage *req, Empty *resp)
{
    printf("HandleInGameAnswer - player: %i, answer: %.*s\n", client_id, req->answer.len, req->answer.data);

    Client *client = &server_data->clients[client_id];
    if (server_data->lobbies.count(client->game_id) == 0)
    {
        // TODO send error PERMISSION_DENIED
        return;
    }

    Lobby *lobby = &server_data->lobbies[client->game_id];
    if (lobby->waiter != &Lobby::waiter_answer)
    {
        return;
    }
    if (client_id != lobby->game.who_can_answer()->id)
    {
        // TODO PERMISSION_DENIED
        return;
    }

    for (int i = 0; i < req->answer.len; i++)
    {
        if (req->answer.data[i] >= 'a' && req->answer.data[i] <= 'z')
            req->answer.data[i] -= 32;
    }

    Broadcaster{this, lobby}.broadcast(&RpcServer::InGamePlayerAnswered, InGameAnswerMessage{req->answer});

    lobby->game.last_answer = req->answer;
    lobby->game.last_answer_client_id = client_id;

    lobby->set_next_stage(&Lobby::stage_respond_to_answer);
}

void RpcServer::HandleInGameChoosePassOrPlay(ClientId client_id, InGameChoosePassOrPlayMessage *req, Empty *resp)
{
    printf("HandleInGameChoosePassOrPlay - player: %i, play?: %i\n", client_id, req->play);
    
    Client *client = &server_data->clients[client_id];
    if (server_data->lobbies.count(client->game_id) == 0)
    {
        // TODO send error PERMISSION_DENIED
        return;
    }

    Lobby *lobby = &server_data->lobbies[client->game_id];
    if (lobby->waiter != &Lobby::waiter_pass_or_play)
    {
        return;
    }
    if (client_id != lobby->game.who_won_faceoff()->id)
    {
        return;
    }

    if (req->play)
    {
        lobby->game.playing_family = lobby->game.faceoff_winning_family;
        Broadcaster{this, lobby}.broadcast(&RpcServer::InGamePlayerChosePassOrPlay, InGameChoosePassOrPlayMessage{true});
    }
    else
    {
        lobby->game.playing_family = 1 - lobby->game.faceoff_winning_family;
        Broadcaster{this, lobby}.broadcast(&RpcServer::InGamePlayerChosePassOrPlay, InGameChoosePassOrPlayMessage{false});
    }

    lobby->set_next_stage(&Lobby::stage_start_play);
}