#ifndef SABOTEUR_SERVER_CARD_H
#define SABOTEUR_SERVER_CARD_H

#include<string>
using namespace std;


class Card {
public:
    string name;
    int type;
    bool is_root;
    explicit Card(string, int);

    enum CardType {
        TUNNEL = 0,
        BLOCK = 1,
        HEAL = 2,
        GOAL = 3
    };

    enum Blockade {
        LAMP = 1,
        PICKAXE,
        TRUCK,
        LAMP_PICKAXE,
        LAMP_TRUCK,
        PICKAXE_TRUCK,
    };
};

class TunnelCard: public Card {
public:
    enum Direction {
        UP = 0,
        DOWN,
        LEFT,
        RIGHT,
        MID,
    };
    bool directions[5];
    explicit TunnelCard(string, int);
    void rotate();
};

class GoalCard: public TunnelCard {
public:
    bool is_gold;
    explicit GoalCard(string, int);
};

class BlockCard: public Card {
public:
    Card::Blockade blockade;
    explicit BlockCard(string, int);
};


#endif //SABOTEUR_SERVER_CARD_H
