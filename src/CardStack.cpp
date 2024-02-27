/*
 * CardArray.cpp
 *
 *  Created on: May 22, 2023
 *      Author: mear
 */

#include "CardStack.h"

#include <stdexcept>

CardStack::CardStack() : nbr_cards(0), bin64(0), mt_rnd_gen(std::mt19937(std::random_device()())) {}

const CardStack CardStack::EMPTY = CardStack();


CardStack::CardStack(uint64_t bin64) : nbr_cards(0), mt_rnd_gen(std::mt19937(std::random_device()())) {
	this->bin64 = bin64 & (~0ull << (64 - Card::N_CARDS) >> (64 - Card::N_CARDS));
	for (Cid id = 0; id < Card::N_CARDS; id++)
		if (this->bin64 & (1ull << id)) {
			append(Card(id));
			nbr_cards++;
		}
}

CardStack::CardStack(Cid ids_arr[], int nbr_cards) : nbr_cards(0), bin64(0), mt_rnd_gen(std::mt19937(std::random_device()()))
{
	for(int i = 0; i < nbr_cards; i++){
		append(Card(ids_arr[i]));
	}
}

CardStack& CardStack::append(const Card &c) {
#ifdef DEBUG
	if (c == Card::NONE)
		throw std::invalid_argument("Card::NONE can't be appended.");
	if (nbr_cards == Card::N_CARDS)
		throw std::invalid_argument("CardStack is full!");
#endif
	cards_arr[nbr_cards++] = c;
	bin64 |= 1ull << c.id;
	return *this;
}

Card CardStack::pop() {
	if (nbr_cards) {
		nbr_cards--;
		bin64 &= ~(1ull << cards_arr[nbr_cards].id); // Not ok for stack with multiple of same card
		return cards_arr[nbr_cards];
	}
	return Card::NONE;
}

std::string CardStack::to_string() const {
	std::string out = "";
	for (int i = 0; i < nbr_cards; i++)
			out += (i != 0) ? " " : "" + cards_arr[i].to_string();
	return out;
}

Card CardStack::at(int i) const {
#ifdef DEBUG
	if ( i >= (int)nbr_cards || -i > (int)nbr_cards )
		throw std::invalid_argument("CardStack::at : Out of bound index: " + std::to_string(i));
#endif
	i = (i >= 0) ? i : nbr_cards + i;
	return cards_arr[i];
}

void CardStack::clear() {
	nbr_cards = 0;
	bin64 = 0;
}

Hand CardStack::to_Hand() const {
	return Hand(bin64);
}

int CardStack::get_nbr_cards() const {
	return nbr_cards;
}

CardStack& CardStack::append(const Card card_arr[], int nbr_cards) {
	for(int i = 0; i < nbr_cards; i++)
		append(card_arr[i]);
	return *this;
}

CardStack& CardStack::shuffle() {
	if (!nbr_cards)
		return *this;
	for (int i = 0; i < nbr_cards - 1; i++) {
		std::uniform_int_distribution<int> uint_rnd_dist(i, nbr_cards - 1);
		int j = uint_rnd_dist(mt_rnd_gen);
		if (j != i) {
			Card tmp = cards_arr[i];
			cards_arr[i] = cards_arr[j];
			cards_arr[j] = tmp;
		}
	}
	return *this;
}

CardStack CardStack::top(int n) const {
#ifdef DEBUG
	if ( n > nbr_cards)
		throw std::invalid_argument("CardStack::top : n is larger than stack size.");
#endif
	CardStack out;
	for(int i = nbr_cards - n; i < nbr_cards; i++){
		out.append(cards_arr[i]);
	}
	return out;
}
