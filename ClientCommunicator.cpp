#include "ClientCommunicator.h"


ClientCommunicator::ClientCommunicator(Client * client, vector<Game *> * games) {
    this->client = client;
    this->games = games;
}

void ClientCommunicator::handle_client_message() {
    bool close_conn = false;
    try {
        int message = receive_int(client);
        switch (message) {
            case ClientCommunicator::REQUEST_GAMES:
                send_games();
                break;
            case ClientCommunicator::CREATE_ROOM:
                create_room();
                break;
            case ClientCommunicator::JOIN_ROOM:
                cout << "Join room" << endl;
                join_room();
                break;
            case ClientCommunicator::CHAT_MESSAGE:
                handle_chat_message();
                break;
            case ClientCommunicator::DRAW_CARD:
                send_card_to_hand();
                break;
            case ClientCommunicator::ADD_CARD_TO_BOARD:
                handle_card_to_board();
                break;
            case ClientCommunicator::BLOCK:
                handle_block_card();
                break;
            case ClientCommunicator::HEAL:
                handle_heal_card();
                break;
            case ClientCommunicator::CLOSE_CONNECTION:
                send_int(client, ClientCommunicator::CLOSE_CONNECTION);
                break;
            default:
                break;
        }
    } catch (ConnectionClosedException &e) {
        close_conn = true;
    } catch (ConnectionBrokenException &e) {
        cout << "read() or write() failed" << endl;
        close_conn = true;
    }
    if (close_conn)
        close_connection_and_game(client);
}

int ClientCommunicator::receive_int(Client * client) {
    int temp;
    int result = 0;
    int got, left = sizeof(int);

    client->pollfd_mutex.lock();
    if (client->active) {
        while ((got = read(client->client_fd->fd, &temp, left)) > 0) {
            left -= got;
            result += temp;
            if (left <= 0) break;
        }
    }
    client->pollfd_mutex.unlock();

    result = ntohl(result);
    if (got < 0) throw ConnectionBrokenException();
    else if (got == 0) throw ConnectionClosedException();
    return result;
}

string ClientCommunicator::receive_text(Client * client) {
    int length = receive_int(client);
    int got, buffer_size = 20;
    int all = 0;
    char buffer[buffer_size];
    string text = "";

    client->pollfd_mutex.lock();
    if (client->active) {
        while ((got = read(client->client_fd->fd, buffer, min(buffer_size - 1, length - all))) > 0) {
            buffer[got] = '\0';
            text += buffer;
            all += got;
            if (all >= length) break;
        }
    }
    client->pollfd_mutex.unlock();

    if (got < 0) throw ConnectionBrokenException();
    else if (got == 0) throw ConnectionClosedException();
    return text;
}

void ClientCommunicator::send_buffer(Client * client, char * buffer, int length) {
    int sent, left = length;
    client->pollfd_mutex.lock();
    if (client->active) {
        while ((sent = write(client->client_fd->fd, buffer + (length - left), left)) > 0) {
            left -= sent;
            if (left <= 0) break;
        }
    }
    client->pollfd_mutex.unlock();

    if (sent < 0) throw ConnectionBrokenException();
    else if (sent == 0) throw ConnectionClosedException();
}

void ClientCommunicator::send_int(Client * client, int number) {
    number = htonl(number);
    char * buffer = new char[sizeof(int)]();
    for (int i = 0; i < sizeof(int); i++) {
        buffer[i] = (number >> (i * 8)) & 0xFF;
    }
    send_buffer(client, buffer, sizeof(int));
}

void ClientCommunicator::send_text(Client * client, string text, int length) {
    send_int(client, length);
    char * buffer = new char[length]();
    strncpy(buffer, text.c_str(), length);
    send_buffer(client, buffer, length);
}

void ClientCommunicator::send_int_to_all(vector <Client *> recipients, int number) {
    bool close_conn;
    for (auto &recipient : recipients) {
        close_conn = false;
        try {
            send_int(recipient, number);
        } catch (ConnectionClosedException &e) {
            close_conn = true;
        } catch (ConnectionBrokenException &e) {
            cout << "write() failed" << endl;
            close_conn = true;
        }
        if (close_conn)
            close_connection_and_game(recipient);
    }
}

void ClientCommunicator::send_text_to_all(vector <Client *> recipients, string text, int length) {
    bool close_conn;
    for (auto &recipient : recipients) {
        close_conn = false;
        try {
            send_text(recipient, text, length);
        } catch (ConnectionClosedException &e) {
            close_conn = true;
        } catch (ConnectionBrokenException &e) {
            cout << "write() failed" << endl;
            close_conn = true;
        }
        if (close_conn)
            close_connection_and_game(recipient);
    }
}

void ClientCommunicator::handle_chat_message() {
    string chat_message = receive_text(client);
    cout << "Sending message: " << chat_message << endl;
    send_chat_message(chat_message);
}

void ClientCommunicator::send_chat_message(string chat_message) {
    send_int_to_all(client->game->players, ClientCommunicator::CHAT_MESSAGE);
    send_text_to_all(client->game->players, chat_message, chat_message.size());
}

void ClientCommunicator::create_room() {
    receive_username();
    string gameName = receive_text(client);
    Game * game = new Game(gameName);
    client->game = game;
    game->players.push_back(client);
    games->push_back(game);
    send_players(game);
}

void ClientCommunicator::join_room() {
    receive_username();
    int gameNumber = receive_int(client);
    Game * game = games->at(gameNumber);
    if (username_repeated(game, client->username)) {
        send_error_message(client, "W tym pokoju istnieje juz gracz o takiej nazwie.");
    }
    else {
        client->game = game;
        game->players.push_back(client);
        send_players(game);
        send_new_player_to_others(game);
        if (game->players.size() == game->ROOM_SIZE) {
            start_game(game);
        }
    }
}

void ClientCommunicator::start_game(Game * game) {
    send_int_to_all(game->players, ClientCommunicator::START_GAME);
    game->activate_first();
    send_player_activation(game);
    int * roles = choose_roles_for_players(game->ROOM_SIZE, game->NUM_OF_SABOTEURS);
    for (int k = 0; k < game->players.size(); k++) {
        auto player = game->players[k];
        if (roles[k] == 1)
            send_role_to_player(player, "Sabotazysta");
        else if (roles[k] == 0)
            send_role_to_player(player, "Dobry krasnal");
        for (int i = 0; i < 5; i++) {
            send_card_to_player(player, game);
        }
    }
    delete roles;
}

int * ClientCommunicator::choose_roles_for_players(int all_dwarfs_num, int saboteurs_num) {
    int * roles = new int[all_dwarfs_num];
    for (int i = 0; i < all_dwarfs_num; i++) {
        if (i < saboteurs_num)
            roles[i] = 1;
        else
            roles[i] = 0;
    }
    random_shuffle(&roles[0], &roles[all_dwarfs_num - 1]);
    return roles;
}

void ClientCommunicator::send_card_to_player(Client * player, Game * game) {
    send_int(player, ClientCommunicator::DRAW_CARD);
    Card * card = game->draw_card();
    send_int(player, card->type);
    send_text(player, card->name, card->name.size());
    player->add_card(card);
}

void ClientCommunicator::receive_username() {
    string username = receive_text(client);
    client->username = username;
}

void ClientCommunicator::send_players(Game * game) {
    for (auto &player: game->players) {
        send_int(client, ClientCommunicator::ADD_PLAYER);
        send_text(client, player->username, player->username.size());
    }
}

void ClientCommunicator::send_new_player_to_others(Game * game) {
    for (auto &player: game->players) {
        if (player != client) {
            send_int(player, ClientCommunicator::ADD_PLAYER);
            send_text(player, client->username, client->username.size());
        }
    }
}

void ClientCommunicator::send_games() {
    send_int(client, ClientCommunicator::REQUEST_GAMES);
    send_int(client, (*games).size());
    for (auto &game: *games) {
        send_text(client, game->name, game->name.size());
    }
}

void ClientCommunicator::send_role_to_player(Client * client, string role) {
    send_int(client, ClientCommunicator::SET_ROLE);
    send_text(client, role, role.size());
}

bool ClientCommunicator::username_repeated(Game *game, string username) {
    bool repeated = false;
    for (auto &player: game->players) {
        if (player->username.compare(username) == 0) {
            repeated = true;
            break;
        }
    }
    return repeated;
}

void ClientCommunicator::send_error_message(Client * client, string error_message) {
    send_int(client, ClientCommunicator::INCORRECT_ACTION);
    send_text(client, error_message, error_message.size());
}

void ClientCommunicator::send_player_activation(Game * game) {
    send_int_to_all(game->players, ClientCommunicator::ACTIVATE_PLAYER);
    Client * active_player = game->get_active_player();
    string username = active_player->username;
    send_text_to_all(game->players, username, username.size());
    if (game->has_empty_pile())
        send_error_message(active_player, "Karty sie skonczyly");
    else
        send_card_to_player(active_player, game);
}

void ClientCommunicator::send_card_to_hand() {
    Game * game = client->game;
    if (!game->is_active_player(client)) {
        send_error_message(client, "Nie jestes aktywnym graczem. Opanuj sie!");
    } else if (game->has_empty_pile()) {
        send_error_message(client, "Karty sie skonczyly");
    } else {
        send_card_to_player(client, game);
    }
}

void ClientCommunicator::handle_card_to_board() {
    int card_index = receive_int(client);
    int x = receive_int(client);
    int y = receive_int(client);
    bool is_rotated = (bool)receive_int(client);
    Game * game = client->game;
    if (!game->is_active_player(client)) {
        send_error_message(client, "Nie jestes aktywnym graczem. Opanuj sie!");
    } else if (client->is_blocked()) {
        send_error_message(client, "Jestes zablokowany. Nie mozesz klasc kart na mapie");
    } else {
        try {
            TunnelCard * card = (TunnelCard *) client->get_card_by_index(card_index);
            game->play_tunnel_card(card, x, y, is_rotated);
            client->remove_card_by_index(card_index);
            send_board_card(game->players, card, x, y, is_rotated);
            send_used_card(card_index);
            send_player_activation(game);
            if (client->game->is_finished()) {
                close_game(client->game, "Dobre krasnale wygraly! Koniec gry");
            }
        } catch (NoCardException &e) {
            send_error_message(client, "Nie masz takiej karty");
        } catch (IncorrectMoveException & e) {
            send_error_message(client, "Niepoprawny ruch. Sprobuj ponownie");
        }
    }
}


void ClientCommunicator::send_board_card(vector<Client *> players, Card * card, int x, int y, bool is_rotated) {
    send_int_to_all(players, ClientCommunicator::ADD_CARD_TO_BOARD);
    send_text_to_all(players, card->name, card->name.size());
    send_int_to_all(players, x);
    send_int_to_all(players, y);
    send_int_to_all(players, is_rotated);
}


void ClientCommunicator::send_used_card(int card_index) {
    send_int(client, ClientCommunicator::REMOVE_CARD_FROM_HAND);
    send_int(client, card_index);
}


void ClientCommunicator::handle_block_card() {
    int card_index = receive_int(client);
    int player_index = receive_int(client);
    Game * game = client->game;
    if (!game->is_active_player(client)) {
        send_error_message(client, "Nie jestes aktywnym graczem. Opanuj sie!");
    } else {
        try {
            BlockCard * card = (BlockCard *) client->get_card_by_index(card_index);
            string player_name = game->play_block_card(card, player_index);
            send_action_card(ClientCommunicator::BLOCK, game->players, card, player_name);
            client->remove_card_by_index(card_index);
            send_used_card(card_index);
            send_player_activation(game);
        } catch (NoCardException &e) {
            send_error_message(client, "Nie masz takiej karty");
        } catch (IncorrectMoveException & e) {
            send_error_message(client, "Niepoprawny ruch. Sprobuj ponownie");
        }
    }
}


void ClientCommunicator::send_action_card(ClientCommunicator::MessageCode messageCode, vector<Client *> players,
      ActionCard * card, string player_name) {
    send_int_to_all(players, messageCode);
    send_int_to_all(players, card->blockade);
    send_text_to_all(players, player_name, player_name.size());
}


void ClientCommunicator::handle_heal_card() {
    int card_index = receive_int(client);
    int player_index = receive_int(client);
    Game * game = client->game;
    if (!game->is_active_player(client)) {
        send_error_message(client, "Nie jestes aktywnym graczem. Opanuj sie!");
    } else {
        try {
            HealCard * card = (HealCard *) client->get_card_by_index(card_index);
            string player_name = game->play_heal_card(card, player_index);
            send_action_card(ClientCommunicator::HEAL, game->players, card, player_name);
            client->remove_card_by_index(card_index);
            send_used_card(card_index);
            send_player_activation(game);
        } catch (NoCardException &e) {
            send_error_message(client, "Nie masz takiej karty");
        } catch (IncorrectMoveException & e) {
            send_error_message(client, "Niepoprawny ruch. Sprobuj ponownie");
        }
    }
}


void ClientCommunicator::close_connection_and_game(Client * client) {
    client->close_connection();
    if (client->game != nullptr) {
        client->leave_game();
        close_game(client->game, client->username + " opuscil gre. Koniec gry!");
        client->game = nullptr;
    }
}


void ClientCommunicator::close_game(Game * game, string message) {
    bool close_conn;
    for (auto &player: game->players) {
        close_conn = false;
        try {
            send_int(player, ClientCommunicator::END_GAME);
            send_text(player, message, message.size());
            send_int(player, ClientCommunicator::CLOSE_CONNECTION);
        } catch (ConnectionClosedException &e) {
            close_conn = true;
        } catch (ConnectionBrokenException &e) {
            cout << "write() failed" << endl;
            close_conn = true;
        }
        if (close_conn) {
            player->close_connection();
        }
        player->game = nullptr;
    }
    games->erase(remove(games->begin(), games->end(), game), games->end());
}