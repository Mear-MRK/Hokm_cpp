/*
 * Agent.h
 *
 *  Created on: May 21, 2023
 *      Author: mear
 */

#ifndef AGENT_HPP_
#define AGENT_HPP_

#include <array>
#include <atomic>

#include "GameConfig.h"
#include "Card.h"
#include "Hand.h"
#include "CardStack.h"
#include "History.h"
#include "State.h"

class Agent
{
private:
	static std::atomic<int> next_id;

protected:
	int player_id{-1};
	int team_id{-1};
	std::string name{""};
	Hand hand;

public:
	Agent();

	virtual ~Agent(){};

	virtual void init_game(){};

	virtual void init_round(const Hand &hand) { this->hand = hand; };

	virtual Suit call_trump(const CardStack &) = 0;

	virtual Card act(const State &, const History &) = 0;

	virtual void trick_result(const State &, const std::array<int, Hokm::N_TEAMS> &){};

	virtual void reset(){};

	virtual void fin_game(){};

	virtual void info(const std::string &){};

	int get_id() const;

	int get_team() const;

	Hand get_hand() const;

	std::string get_name() const;

	void set_name(std::string);

	static void reset_id() { next_id.store(0); };
};

#endif /* AGENT_HPP_ */
