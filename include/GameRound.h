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
    CardStack collect = CardStack::EMPTY;
    std::array<Hand, Hokm::N_PLAYERS> hand;
    std::array<Agent *, Hokm::N_PLAYERS> agent;
    std::array<int, Hokm::N_TEAMS> team_scores;
    std::mt19937 mt_rnd_gen;
	
	public:
    State state;
    std::array<CardStack, Hokm::N_PLAYERS> stack;
    int winner_team;
    int trump_team;
    int opening_player;
    int kot;
    bool show_info = false;
    int turn_sleep_ms = 500;
    int rnd_sleep_ms = 1500;
	std::string name[Hokm::N_PLAYERS];

    GameRound(std::array<Agent *, Hokm::N_PLAYERS>);

    int trick();

    int play(int round_win_score = Hokm::RND_WIN_SCORE);

    void reset();

    void trump_call();

    void deal_n_init();

    void broadcast_info(std::string info_str, int exclude = -1);

    // NEW: expose current hand scores for snapshotting on resume
    const std::array<int, Hokm::N_TEAMS>& get_hand_scores() const { return team_scores; }

    // NEW: expose a player's current hand string (for convenience)
    const Hand& get_player_hand(int pid) const { return hand[pid]; }
};
