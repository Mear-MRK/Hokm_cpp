#pragma once

#include <random>
#include <utility>

#include "GameConfig.h"
#include "Agent.h"
#include "ProbHand.h"

class SoundAgent: public Agent {

	std::mt19937 mt_rnd_gen;

	Hand Ha, Hb, Hc, Hab, Hbc, Hca, Habc;
	int Nu_a, Nu_b, Nu_c;
	int Ncards_a, Ncards_b, Ncards_c;

	ProbHand Pa, Pb, Pc;

	double prob_floor, trump_prob_cap, prob_ceiling;

	int last_ord;
	Suit last_led;

	void update_oth_hands(int pl_id, const Card& pl_c, Suit led);
	void updateHandsNcards();

	void updateProbs();

	std::pair<Card, double> prb_act(const Hand &ps_hi, const Hand &ps_oth, int N_ex, Suit led, Suit trump
	, bool critical = false) const;


public:

	SoundAgent(double prob_floor = 0.5, double trump_prob_cap = 1, double prob_ceiling = 1);
	
	void set_probs(double prob_floor, double trump_prob_cap = 1, double prob_ceiling = 1);

	Suit call_trump(const CardStack& first_5cards) override;

	void init_round(const Hand& hand) override;

	Card act(const State&, const History&) override;

	void reset() override;

};