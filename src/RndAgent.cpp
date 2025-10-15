/*
 * Agent.cpp
 *
 *  Created on: May 21, 2023
 *      Author: mear
 */

#include "RndAgent.h"

#include "utils.h"

RndAgent::RndAgent() : Agent(), mt_rnd_gen(std::mt19937(std::random_device()())) {
	name = "RN_" + std::to_string(player_id);
}


Suit RndAgent::call_trump(const CardStack &first_5cards) {
	return first_5cards.at(std::uniform_int_distribution<int>(0, 4)(mt_rnd_gen)).su;
}


Card RndAgent::act(const State &state, const History &) {
	LOG(name + " internal hand: " + hand.to_string());
	int ind;
	if (state.led == Card::NON_SU || hand.len[state.led] == 0) {
		int s;
		Suit non_zero[Card::N_SUITS];
		int nbr_nn = 0;
		for (Suit su = 0; su < Card::N_SUITS; su++) {
			if (hand.len[su] != 0) {
				non_zero[nbr_nn++] = su;
			}
		}
		s = non_zero[std::uniform_int_distribution<int>(0, nbr_nn - 1)(
				mt_rnd_gen)];
		ind = std::uniform_int_distribution<int>(0, hand.len[s] - 1)(
				mt_rnd_gen);
		return hand.pop(s, ind);
	}
	ind = std::uniform_int_distribution<int>(0, hand.len[state.led] - 1)(
			mt_rnd_gen);
	return hand.pop(state.led, ind);
}
