#pragma once

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

class GameRound
{
	friend class LearningGame;

private:
	int round_id;
	History hist;
	Deck deck;
	std::array<CardStack, Hokm::N_PLAYERS> stack;
	std::array<Hand, Hokm::N_PLAYERS> hand;
	std::array<Agent *, Hokm::N_PLAYERS> agent;
	std::array<int, Hokm::N_TEAMS> team_scores;
	std::mt19937 mt_rnd_gen;

public:
	State state;
	int winner_team;
	int trump_team;
	int opening_player;
	int kot;
	bool show_info = false;
	int turn_sleep_ms = 500;
	int rnd_sleep_ms = 1500;

	GameRound(std::array<Agent *, Hokm::N_PLAYERS>);

	int trick();

	int play(int round_win_score = Hokm::RND_WIN_SCORE);

	void reset();

	void trump_call();

	void deal_n_init();

	void broadcast_info(std::string info_str, int exclude = -1);
};
