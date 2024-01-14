/*
 * Deck.h
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#ifndef DEBUG_DECK_HPP_
#define DEBUG_DECK_HPP_

#include <random>

#include "Card.h"
#include "CardStack.h"

class Deck {

private:
	bool outside;
	Card cards[Card::N_CARDS];
	std::mt19937 mt_rnd_gen;


public:

	Deck();
	Deck(const CardStack&);
	void shuffle();
	void shuffle_bag(int nbr_players);

	void shuffle_deal(CardStack* stacks, int nbr_players);

	void deal(CardStack* stacks, int);

};

#endif /* DEBUG_DECK_HPP_ */
