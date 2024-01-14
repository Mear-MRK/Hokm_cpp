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
	Agent* agents[Hokm::N_PLAYERS];
	std::unique_ptr<GameRound> round;
	int team_scores[Hokm::N_TEAMS]{0};
public:
	InteractiveGame(std::string ag_typs, bool show_hand = false);
	~InteractiveGame();

	int play(int win_score = Hokm::WIN_SCORE, int round_win_score = Hokm::RND_WIN_SCORE);
	void broadcast_info(std::string info_str);
};

#endif /* INTERACTIVEGAME_H_ */
