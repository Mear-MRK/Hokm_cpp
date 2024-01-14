/*
 * CardArray.h
 *
 *  Created on: May 22, 2023
 *      Author: mear
 */

#ifndef CARDSTACK_H_
#define CARDSTACK_H_


#include <string>
#include <random>

#include "Card.h"
#include "Hand.h"

class CardStack {
	Card cards_arr[Card::N_CARDS];
	int nbr_cards;
	std::mt19937 mt_rnd_gen;
public:
	uint64_t bin64;

	CardStack();
	CardStack(std::uint64_t bin64);
	CardStack(CID ids_arr[], int nbr_cards);

	void append(const Card &c);
	void append(const Card card_arr[], int nbr_cards);
	Card pop();

	void shuffle();

	Card at(int i) const ;

	CardStack top(int n) const;

	void reset();
	std::string to_string() const;

	Hand to_Hand() const;

	int get_nbr_cards() const;

	static const CardStack EMPTY;
};


#endif /* CARDSTACK_H_ */
