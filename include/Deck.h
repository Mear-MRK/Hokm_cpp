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
#include "GameConfig.h"

class Deck {

private:
	bool outside;
	Card cards[Card::N_CARDS];
	std::mt19937 mt_rnd_gen;


public:

	Deck();
	Deck(const CardStack&);

	CardStack to_cardStack() const;

	void shuffle(int cmplx=3);
	void shuffle_rnd();
	void shuffle_overhand(int cmplx=1);
	void shuffle_riffle(int cmplx=1);
	void shuffle_rnd_bag();
	void shuffle_deal(CardStack* stacks, int opening_player, int cmplx=3);

	void deal(CardStack* stacks, int opening_player);

};

#endif /* DEBUG_DECK_HPP_ */
