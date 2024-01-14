/*
 * InteractiveGame.cpp
 *
 *  Created on: May 27, 2023
 *      Author: mear
 */

#include "InteractiveGame.h"

#include <iostream>
#include <thread>
#include <chrono>

#include "InteractiveAgent.h"
#include "RemoteInterAgent.h"
#include "SoundAgent.h"
#include "utils.h"

static Agent* newAgentFrom(char ag_typ, int id, bool show_hand = true)
{
	switch(ag_typ)
	{
		case 's':
			return new SoundAgent(id);
		case 'r':
			return new RemoteInterAgent(id, show_hand);
		case 'i':
			return new InteractiveAgent(id, show_hand);
		default:
			return nullptr;
	}
}

InteractiveGame::InteractiveGame(std::string ag_typs, bool show_hand)
{
	for(int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		agents[pl] = newAgentFrom(ag_typs[pl], pl, show_hand);

	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		agents[pl]->init_game();

	round = std::unique_ptr<GameRound>(new GameRound(agents));
}

InteractiveGame::~InteractiveGame()
{

	for (int pl = 0; pl < 4; pl++)
		delete agents[pl];
}

int InteractiveGame::play(int win_score, int round_win_score)
{

	int round_winner_team = -1;
	for (int r = 1; r <= 2 * win_score - 1; r++)
	{

		std::string round_hdr_str = "/INF=== Round " + std::to_string(r) + " ===";
		LOG(round_hdr_str);
		broadcast_info(round_hdr_str);

		round->reset();
		round->set_round_id(r);
		std::string start_round_str =
			"/ALRWaiting for " + agents[round->start_player]->get_name() + " to call the trump suit... ";
		broadcast_info(start_round_str);

		round->trump_call();

		broadcast_info("/INF" + agents[round->start_player]->get_name() + " called " + Card::SU_STR[round->state.trump] + " as trump.");
		broadcast_info("/HED/TRM" + Card::SU_STR[round->state.trump]);
		
		round->deal_n_init();

		round_winner_team = round->play(true, round_win_score);

		team_scores[round_winner_team] += 1 + round->kot;

		std::string end_round_str =
			"/ALR=== Round " +
			std::to_string(r) + " winner team: " + std::to_string(round_winner_team) + "  (Kot: " +
			std::to_string(round->kot) + ") ===";
		LOG(end_round_str);
		broadcast_info(end_round_str);
		std::string game_scr_str =
			std::to_string(team_scores[0]) + ":" + std::to_string(team_scores[1]);
		broadcast_info("/HED/GSC" + game_scr_str);
		std::this_thread::sleep_for(std::chrono::seconds(2));
		broadcast_info("/HED/RSC0:0");
		broadcast_info("/HED/TRM");
		broadcast_info("/ALR");

		if (team_scores[round_winner_team] >= win_score)
			break;
	}

	// int winner_teams[Hokm::N_TEAMS] = {-1};
	// int max_score = 0;
	// for(int team = 0; team < Hokm::N_TEAMS; team++)
	// 	if (team_scores[team] > max_score){
	// 		max_score = team_scores[team];
	// 	}
	// int n = 0;
	// for(int team = 0; team < Hokm::N_TEAMS; team++){
	// 	if (team_scores[team] == max_score)
	// 		winner_teams[n++] = team;
	// }

	std::string endgame_str = "";
	// if (n == 1) {
	endgame_str = "*** Team " + std::to_string(round_winner_team) + " is the winner of the game. ***";

	LOG(endgame_str);

	broadcast_info("/ALR" + endgame_str);
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		agents[pl]->end_game();

	return round_winner_team;
}

void InteractiveGame::broadcast_info(std::string info_str)
{
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		agents[pl]->info(info_str);
}
