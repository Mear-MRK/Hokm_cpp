#pragma once

#include <string>
#include <random>

#include "Card.h"
#include "Hand.h"

class CardStack {
	Card cards_arr[Card::N_CARDS];
	int nbr_cards;
public:
	std::uint64_t bin64;
private:
	std::mt19937 mt_rnd_gen;

public:
	CardStack();
	CardStack(std::uint64_t bin64);
	CardStack(const Cid ids_arr[], int nbr_cards);
	CardStack(const Card cards[], int nbr_cards);

	CardStack& append(const Card &c);
	CardStack& append(const Card card_arr[], int nbr_cards);
	CardStack& append(const Hand& );
	
	Card pop();

	CardStack& shuffle();

	Card at(int i) const ;

	CardStack top(int n) const;
	CardStack bottom(int n) const;

	void clear();

	std::string to_string() const;

	Hand to_Hand() const;

	int get_nbr_cards() const;

	static const CardStack EMPTY;
};
