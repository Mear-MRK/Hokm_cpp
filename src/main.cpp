/*
 * main.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <signal.h>

#include "InteractiveGame.h"
#include "LearningGame.h"
#include "utils.h"

#ifdef DEBUG
// void utils_test();
// void Card_test();
// void CardStack_test();
// void Hand_test();
// void Deck_test();
// void State_test();
// void History_test();
#endif

int main(int argc, char *argv[])
{
	struct sigaction sig_act = {SIG_IGN};
	sigaction(SIGPIPE, &sig_act, NULL);
	srand(time(NULL));

	// int nbr_episodes = 10;
	// int nbr_probs = 5;
	// double min_prob = 0;
	// double max_prob = 1;
	// if (argc > 1) {
	// 	nbr_episodes = std::stoul(argv[1]);
	// }
	// if (argc > 2) {
	// 	nbr_probs = std::stoul(argv[2]);
	// }
	// if (argc > 3){
	// 	min_prob = std::stod(argv[3]);
	// }
	// if (argc > 4){
	// 	max_prob = std::stod(argv[4]);
	// }

	// LearningGame game{nbr_probs, min_prob, max_prob};

	// game.play(nbr_episodes);

#ifdef DEBUG
	// utils_test();
	// Card_test();
	// CardStack_test();
	// Hand_test();
	// Deck_test();
	// State_test();
	// History_test();
#endif

	std::string agent_types = "srss";
	int game_win_score = Hokm::WIN_SCORE;
	int round_win_score = Hokm::RND_WIN_SCORE;
	switch(argc)
	{
		case 4:
			round_win_score = std::stoi(argv[3]);
		case 3:
			game_win_score = std::stoi(argv[2]);
		case 2:
			agent_types = std::string(argv[1]);
	}

	InteractiveGame game(agent_types, true);
	game.play(game_win_score, round_win_score);

	return 0;
}
