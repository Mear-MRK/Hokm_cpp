/*
 * Game.h
 *
 *  Created on: May 21, 2023
 *      Author: mear
 */

#ifndef GAMEROUND_H_
#define GAMEROUND_H_

#include <random>
#include <memory>
#include <array>

#include "GameConfig.h"
#include "Agent.h"
#include "Card.h"
#include "CardStack.h"
#include "Hand.h"
#include "Deck.h"
#include "History.h"
#include "State.h"

class Agent;

class GameRound {
	friend class LearningGame;


private:

	int round_id;
	History hist;
	Deck deck;
	CardStack stacks[Hokm::N_PLAYERS];
	Hand hands[Hokm::N_PLAYERS];
	Agent** agents;
	std::array<int, Hokm::N_TEAMS> team_scores;
	std::mt19937 mt_rnd_gen;

public:
	State state;
	int winner_team;
	int trump_team;
	int start_player;
	int kot;

	GameRound(Agent*[]);

	int trick(bool show_info = false);

	int play(bool show_info = false, int round_win_score=Hokm::RND_WIN_SCORE);

	void set_round_id(int round_id);

	void reset();

	void trump_call();

	void deal_n_init();

	void broadcast_info(std::string info_str);

};

#endif /* GAMEROUND_H_ */
