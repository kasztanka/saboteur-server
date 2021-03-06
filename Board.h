#ifndef SABOTEUR_SERVER_BOARD_H
#define SABOTEUR_SERVER_BOARD_H


#include "Card.h"
#include <list>

class Board {
public:
    static const int ROWS = 5;
    static const int COLS = 10;
    bool validate_tunnel_card(TunnelCard *, int, int);
    void set_card(TunnelCard * card, int, int);
    TunnelCard * get_card(int, int);
    bool card_has_connection_to_root(TunnelCard *, int, int);
private:
    TunnelCard * cards[ROWS][COLS];
    bool is_spot_for_new_card_free(int, int);
    bool card_fits_to_neighbour(TunnelCard *, TunnelCard::Direction, bool);
    bool coordinates_are_on_board(int, int);
    bool card_fits_to_neighbours(TunnelCard *, int, int);
    void get_neighbours_coordinates(int[4][2], int, int);
};


#endif //SABOTEUR_SERVER_BOARD_H
