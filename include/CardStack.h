#pragma once

#include <string>
#include <random>

#include "Card.h"
#include "Hand.h"

class CardStack {
	Card cards_arr[Card::N_CARDS];
	int nbr_cards;
	std::mt19937 mt_rnd_gen;
public:
	std::uint64_t bin64;

	CardStack();
	CardStack(std::uint64_t bin64);
	CardStack(Cid ids_arr[], int nbr_cards);

	CardStack& append(const Card &c);
	CardStack& append(const Card card_arr[], int nbr_cards);
	
	Card pop();

	CardStack& shuffle();

	Card at(int i) const ;

	CardStack top(int n) const;

	void clear();

	std::string to_string() const;

	Hand to_Hand() const;

	int get_nbr_cards() const;

	static const CardStack EMPTY;
};
