/*
 * Agent.h
 *
 *  Created on: May 21, 2023
 *      Author: mear
 */

#ifndef AGENT_HPP_
#define AGENT_HPP_

#include <array>

#include "GameConfig.h"
#include "Card.h"
#include "Hand.h"
#include "CardStack.h"
#include "History.h"
#include "State.h"

class Agent
{

protected:
	int player_id{-1};
	int team_id{-1};
	std::string name;
	Hand *hand{nullptr};

public:
	Agent(int player_id, std::string name) : player_id(player_id), team_id(player_id % Hokm::N_TEAMS),
											 name(name), hand(nullptr){};
	virtual ~Agent(){};

	virtual void init_game(){};

	virtual void init_round(Hand &hand) { this->hand = &hand; };

	virtual Suit call_trump(const CardStack &) = 0;

	virtual Card act(const State &, const History &) = 0;

	virtual void trick_result(const State &, const std::array<int, Hokm::N_TEAMS> &team_scores){};

	virtual void reset(){};

	virtual void end_game(){};

	virtual void info(const std::string &info_str){};

	virtual Hand &get_hand()
	{
		return *hand;
	};

	virtual std::string get_name() const { return name; };
};

#endif /* AGENT_HPP_ */
