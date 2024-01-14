#include "GameRound.h"

#include <iostream>
#include <random>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "Card.h"
#include "CardStack.h"
#include "Deck.h"
#include "Hand.h"
#include "utils.h"
#include "table.h"

GameRound::GameRound(Agent *agents[]) : round_id(-1),
										agents(agents),
										kot(0),
										winner_team(-1),
										trump_team(-1),
										team_scores({0}),
										start_player(-1),
										mt_rnd_gen(std::mt19937(std::random_device()()))
{
}

void GameRound::set_round_id(int round_id)
{
	this->round_id = round_id;
}

void GameRound::reset()
{
	state.reset();
	hist.reset();
	std::uniform_int_distribution<int> four_rnd(0, 3);
	if (winner_team == -1)
	{
		start_player = four_rnd(mt_rnd_gen);
	}
	else if (winner_team != trump_team)
	{
		start_player = (start_player + 1) % Hokm::N_PLAYERS;
	}
	state.turn = start_player;
	trump_team = start_player % Hokm::N_TEAMS;
	deck.shuffle_deal(stacks, Hokm::N_PLAYERS);
	stacks[state.turn].shuffle();
	state.trump = -1;
	team_scores.fill(0);
	state.led = Card::NON_SU;
	winner_team = -1;
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		this->agents[pl]->reset();
}

void GameRound::trump_call()
{
	state.trump = this->agents[state.turn]->call_trump(stacks[state.turn].top(5));
}

void GameRound::deal_n_init()
{
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
	{
		hands[pl] = stacks[pl].to_Hand();
		this->agents[pl]->init_round(hands[pl]);
	}
}

void GameRound::broadcast_info(std::string info_str)
{
	for (int l = 0; l < Hokm::N_PLAYERS; l++)
		agents[l]->info(info_str);
}

inline int GameRound::trick(bool show_info)
{
	state.reset();
	LOG(
		"+++ Trick id " << (int)state.trick_id << " turn " << (int)state.turn << " trump " << Card::SU_STR[state.trump] << " +++");

	if (show_info)
		broadcast_info("/INF--- Trick " + std::to_string(state.trick_id + 1) + " ---");

	for (int ord = 0; ord < Hokm::N_PLAYERS; ord++)
	{
		int pl = (state.turn + ord) % Hokm::N_PLAYERS;
		state.ord = ord;

		if (show_info)
			broadcast_info("/ALRWaiting for " + agents[pl]->get_name() + " to play...");

		Card c = agents[pl]->act(state, hist);

		if (show_info)
		{
			broadcast_info("/ALR");
			broadcast_info("/INF" + agents[pl]->get_name() + " played " + c.to_string());
			agents[pl]->info("/HND" + agents[pl]->get_hand().to_string());
		}

#ifdef CHK_VL
		if (!hands[pl].card_is_valid(c, state.led))
			throw std::logic_error(
				"Game::trick : Card " + c.to_string() + " is not a valid act for agent " + std::to_string(pl) + " while led: " + Card::SU_STR[state.led] + ", hand: " + hands[pl].to_string());
#endif

		if (ord == 0)
			state.led = c.su;
		state.table[pl] = c;
		if (show_info)
		{
			broadcast_info("/TBL" + table_str(state.table));
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	Card best_card = state.table[0];
	int best_pl = 0;
	for (int pl = 1; pl < Hokm::N_PLAYERS; pl++)
		if (Card::cmp(state.table[pl], best_card, state.led, state.trump) > 0)
		{
			best_card = state.table[pl];
			best_pl = pl;
		}

	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
	{
		hist.played_cards[pl].append(state.table[pl]);
		LOG( agents[pl]->get_name() << " played " << hist.played_cards[pl].at(-1).to_string() );
	}

	LOG(
		"^^^ Trick id " << (int)state.trick_id << " turn " 
		<< (int)state.turn << " led " << Card::SU_STR[state.led] 
		<< " trump " << Card::SU_STR[state.trump] << " ^^^");

	return best_pl;
}

int GameRound::play(bool show_info, int round_win_score)
{
#ifdef DEBUG
	show_info = true;
#endif

	kot = 0;
	for (int trick_id = 0; trick_id < Hokm::N_TRICKS; trick_id++)
	{
		state.trick_id = trick_id;
#ifdef DEBUG
		for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
			LOG( agents[pl]->get_name() << " hand " << agents[pl]->get_hand().to_string() );
		LOG( "Round " << round_id << ", Trick " << trick_id << ", Turn: " << (int)state.turn << ", Trump: "
				  << Card::SU_STR[state.trump] );
#endif

		int trick_taker = trick(show_info);

		int trick_taker_team = trick_taker % Hokm::N_TEAMS;
		team_scores[trick_taker_team]++;
		state.turn = trick_taker;

		for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
			agents[pl]->trick_result(state, team_scores);

		bool round_finished = false;
		for (int team = 0; team < Hokm::N_TEAMS; team++)
			if (team_scores[team] >= round_win_score)
			{
				round_finished = true;
				winner_team = team;
				break;
			}

		if (show_info)
		{
			broadcast_info("/HED/RSC" + std::to_string(team_scores[0]) + ":" + std::to_string(team_scores[1]));
			broadcast_info("/INFLast table: " + table_str(state.table));
			std::this_thread::sleep_for(std::chrono::seconds(2));
			table_clear(state.table);
			broadcast_info("/TBL" + table_str(state.table));
			if (round_finished)
				broadcast_info("/HND");
		}

		if (round_finished)
		{
			int oth_scr = 0;
			for (int team = 0; team < Hokm::N_TEAMS; team++)
				if (team != winner_team)
					oth_scr += team_scores[team];
			kot = (int)(oth_scr == 0) + (int)((oth_scr == 0) && (winner_team != trump_team));
			LOG(
				"kot: " << kot << ", oth_scr: " << oth_scr << ", winner_team: " << winner_team << ", trump_team: " << trump_team);
			break;
		}
	} // trick_id
	return winner_team;
}
