#pragma once

#include <unordered_map>

#include "common.hpp"
#include "game_state.hpp"
#include "net/net.hpp"
#include "net/generated_rpc_server.hpp"

struct ServerData
{
    std::unordered_map<GameId, Lobby> lobbies;
    std::unordered_map<ClientId, Client> clients;
};

ClientId add_client(ServerData *server_data, SOCKET s)
{
    static ClientId next_client_id = 1;
    next_client_id++;

    if (server_data->clients.size() > MAX_CLIENTS)
    {
        // TODO send error message SERVER_FULL
        return 0;
    }

    Client client;
    client.peer.s = s;
    client.client_id = next_client_id;

    server_data->clients[next_client_id] = client;
    return next_client_id;
}

void remove_client(ServerData *server_data, ClientId client_id)
{
    Client *client = &server_data->clients[client_id];
    if (client->game_id && server_data->lobbies.count(client->game_id))
    {
        Lobby &lobby = server_data->lobbies[client->game_id];
        lobby.remove_player(client_id);
    }
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
    if (ret.len > 0 && ret.data[ret.len - 1] == ' ')
    {
        ret.len -= 1;
    }

    return ret;
}

void RpcServer::HandleListGames(ClientId client_id, ListGamesRequest *req, ListGamesResponse *resp)
{
    uint16_t count = 0;
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
            game_msg.num_players = lobby->num_players();
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
        return;
    }
    
    Lobby *lobby = &server_data->lobbies[req->game_id];
    resp->game.id = req->game_id;
    resp->game.name = lobby->properties.name;
    resp->game.owner = server_data->clients[lobby->properties.owner].username;
    resp->game.num_players = lobby->num_players();
    resp->game.is_self_hosted = lobby->properties.is_self_hosted;
    for (int i = 0; i < lobby->players.len; i++)
    {
        resp->players.push_back({lobby->players[i].id,
                                 lobby->players[i].name,
                                 lobby->players[i].team == 1});
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
        return; // TODO invalid name
    }
    if (owner_name.len == 0)
    {
        return; // TODO invalid name
    }
    if (!req->is_self_hosted)
    {
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

void RpcServer::HandleSwapTeam(ClientId client_id, SwapTeamRequest *req, SwapTeamResponse *resp)
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
    if (!lobby->is_player_in_this_game(req->user_id))
    {
        return; // error requested user isn't in this game
    }

    for (int i = 0; i < lobby->players.len; i++)
    {
        if (lobby->players[i].id == req->user_id)
        {
            lobby->players[i].team = 1 - lobby->players[i].team;
        }
    }
}

void RpcServer::HandleLeaveGame(ClientId client_id, LeaveGameRequest *req, LeaveGameResponse *resp)
{
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
    lobby->remove_player(client->client_id);
    client->game_id = 0;
}

void RpcServer::HandleStartGame(ClientId client_id, StartGameRequest *req, StartGameResponse *resp)
{
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

    lobby->start_game();
    for (int i = 0; i < lobby->players.len; i++)
    {
        Peer *peer = &server_data->clients[lobby->players[i].id].peer;
        StartGame(peer, {req->game_id});
    }

}