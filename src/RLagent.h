#pragma once

#include "GameConfig.h"
#include "Agent.h"
#include <random>

class RLagent : public Agent {
private:
	std::mt19937 mt_rnd_gen;
public:
	RLagent(int player_id);
    
    void init_round(Hand &hand) override;
	Card act(const State&, const History&) override;
	Suit call_trump(const CardStack&) override;

	void trick_result(const State&, const std::array<int, Hokm::N_TEAMS>& team_scores) override;
};
