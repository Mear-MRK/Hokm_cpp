/*
 * InteractiveAgent.h
 *
 *  Created on: Jun 1, 2023
 *      Author: mear
 */

#ifndef INTERACTIVEAGENT_H_
#define INTERACTIVEAGENT_H_

#include <iostream>

#include "Agent.h"

class InteractiveAgent : public Agent {
	bool show_hand;
public:
	InteractiveAgent(bool show_hand = true);

	void init_game() override;
	void init_round(const Hand &hand) override;
	Card act(const State&, const History&) override;
	Suit call_trump(const CardStack&) override;

	void trick_result(const State&, const std::array<int, Hokm::N_TEAMS>& team_scores) override;

	void info(const std::string &info_str) override;

	virtual void output(const std::string &out_str) { std::cout << out_str << std::endl; }
	virtual std::string input(const std::string &prompt) {
		std::cout << prompt;
		std::string inp_str = "";
		std::cin >> inp_str;
		return inp_str;
	}

	
};

#endif /* INTERACTIVEAGENT_H_ */
