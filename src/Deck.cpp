/*
 * Deck.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#include "Deck.h"

#include <iostream>



Deck::Deck() : outside(false), 	mt_rnd_gen(std::mt19937(std::random_device()())) {
	for (int i = 0; i < Card::N_CARDS; i++)
		cards[i] = Card(i);
}

Deck::Deck(const CardStack& out_deck) : outside(true), 	mt_rnd_gen(std::mt19937(std::random_device()())) {
	for (int i = 0; i < out_deck.get_nbr_cards(); i++)
		cards[i] = out_deck.at(i);
}

void Deck::shuffle() {
	for (int i = 0; i < Card::N_CARDS - 1; i++) {
		std::uniform_int_distribution<int> int_rnd_dist(i, Card::N_CARDS - 1);
		int j = int_rnd_dist(mt_rnd_gen);
		if (j != i) {
			Card tmp = cards[i];
			cards[i] = cards[j];
			cards[j] = tmp;
		}
	}
}

void Deck::shuffle_bag(int nbr_players) {
#ifdef DEBUG
	if (nbr_players < 1)
		throw std::invalid_argument("Deck::shuffle_beg nbr_players is less than 1: " + std::to_string(nbr_players));
#endif
	int nbr_delt = Card::N_CARDS / nbr_players;
	for (int s = 0; s < nbr_players - 1; s++) {
		std::uniform_int_distribution<int> int_rnd_dist(0, nbr_players - s - 1);
		int i_min = s * nbr_delt;
		int i_max = i_min + nbr_delt;
		for (int i = i_min; i < i_max; i++) {
			int j = int_rnd_dist(mt_rnd_gen);
			if (j) {
				int i_j = i + j * nbr_delt;
				Card tmp = cards[i];
				cards[i] = cards[i_j];
				cards[i_j] = tmp;
			}
		}
	}
}

void Deck::shuffle_deal(CardStack *stacks, int nbr_players) {
#ifdef DEBUG
	if (nbr_players < 1)
		throw std::invalid_argument("Deck::shuffle_deal nbr_players is less than 1: " + std::to_string(nbr_players));
#endif
	shuffle_bag(nbr_players);
	deal(stacks, nbr_players);
}

void Deck::deal(CardStack *stacks, int nbr_players) {
#ifdef DEBUG
	if (nbr_players < 1)
		throw std::invalid_argument("Deck::deal nbr_players is less than 1: " + std::to_string(nbr_players));
#endif
	int nbr_delt = Card::N_CARDS / nbr_players;
	for(int i = 0; i < nbr_players; i++) {
		stacks[i].clear();
		stacks[i].append(cards + i * nbr_delt, nbr_delt);
	}
}
