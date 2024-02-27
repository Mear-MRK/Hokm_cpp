/*
 * InteractiveGame.h
 *
 *  Created on: May 27, 2023
 *      Author: mear
 */

#ifndef INTERACTIVEGAME_H_
#define INTERACTIVEGAME_H_

#include <memory>

#include "GameConfig.h"
#include "GameRound.h"

class InteractiveGame {
private:
	std::array<Agent*, Hokm::N_PLAYERS>agent;
	std::unique_ptr<GameRound> round;
	std::array<int, Hokm::N_TEAMS>team_scores = {0};
public:
	InteractiveGame(std::string ag_typs, bool show_hand = false);
	~InteractiveGame();

	int play(int win_score = Hokm::WIN_SCORE, int round_win_score = Hokm::RND_WIN_SCORE);
	void broadcast_info(std::string info_str, int exclude = -1);
};

#endif /* INTERACTIVEGAME_H_ */
