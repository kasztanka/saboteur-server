#include "Game.h"
#include "Client.h"


Game::Game(string name) {
    this->name = name;
    this->room_size = 2;
    prepare_deck();
}

Card * Game::draw_card() {
    Card * result = pile_of_cards.back();
    pile_of_cards.pop_back();
    return result;
}

void Game::add_card_to_deck(string name, int quantity) {
    for (int i = 0; i < quantity; i++)
        pile_of_cards.push_back(new TunnelCard(name, Card::TUNNEL));
}

void Game::prepare_deck() {
    add_card_to_deck("UDM", 2);
    add_card_to_deck("DLM", 3);
    random_device rd;
    mt19937 g(rd());
    shuffle(pile_of_cards.begin(), pile_of_cards.end(), g);
}

void Game::activate_first() {
    active_player = players.at(0);
}

string Game::get_active_player_username() {
    return active_player->username;
}

bool Game::is_active_player(Client * client) {
    return active_player == client;
}

bool Game::has_empty_pile() {
    return pile_of_cards.size() == 0;
}
