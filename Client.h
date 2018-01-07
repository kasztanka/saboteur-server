#ifndef SABOTEUR_SERVER_CLIENT_H
#define SABOTEUR_SERVER_CLIENT_H

#include <sys/poll.h>
#include <string>
#include <mutex>
#include <unistd.h>
#include <iostream>
#include "Game.h"

using namespace std;


struct NoCardException : public exception {};


class Client {
private:
    vector<Card *> hand_cards;
    vector<Card::Blockade> blockades;
public:
    pollfd * client_fd;
    mutex pollfd_mutex;
    bool active;
    string username;
    Game * game;
    explicit Client(pollfd * client_fd);
    void addCard(Card *);
    Card * getCardByIndex(int);
    void removeCardByIndex(int);
    bool has_blockade(Card::Blockade);
    void add_blockade(Card::Blockade);
    int remove_blockades(vector<Card::Blockade>);
    void close_connection();
};


#endif //SABOTEUR_SERVER_CLIENT_H
