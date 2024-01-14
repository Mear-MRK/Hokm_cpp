/*
 * SoundAgent.h
 *
 *  Created on: May 29, 2023
 *      Author: mear
 */

#ifndef SOUNDAGENT_H_
#define SOUNDAGENT_H_

#include <random>
#include <utility>

#include "GameConfig.h"
#include "Agent.h"

class SoundAgent: public Agent {

	std::mt19937 mt_rnd_gen;

	Hand Ha, Hb, Hc, Hab, Hbc, Hca, Habc;

	double prob_floor, trump_prob_cap, prob_ceiling;

	int last_ord;
	Suit last_led;

	void update_oth_hands(int pl_id, const Card& pl_c, Suit led);

	std::pair<Card, double> prb_act(const Hand &ps_hi, const Hand &ps_oth, int N_ex, Suit led, Suit trump
	, bool critical = false) const;

public:
	SoundAgent(int player_id, double prob_floor = 0.5, double trump_prob_cap = 1, double prob_ceiling = 1);
	void set_probs(double prob_floor, double trump_prob_cap = 1, double prob_ceiling = 1);

	Suit call_trump(const CardStack& first_5cards) override;

	void init_round(Hand& hand) override;

	Card act(const State&, const History&) override;

	void reset() override;
};

#endif /* SOUNDAGENT_H_ */
