/*
 * LearningGame.h
 *
 *  Created on: May 31, 2023
 *      Author: mear
 */

#ifndef LEARNINGGAME_H_
#define LEARNINGGAME_H_

#include "GameConfig.h"
#include "Agent.h"
#include "GameRound.h"

class LearningGame {
	std::unique_ptr<GameRound> round;
	std::array<Agent *, Hokm::N_PLAYERS> agent;
	int nbr_probs;
	int nbr_stats;
	double *probs;
	double *stats;

	void tweak_floor_trump_probs(int nbr_episodes);
	void tweak_trump_prob_cap(int nbr_episodes);
	void tweak_floor_prob(int nbr_episodes);
	void tweak_floor_prob_vs_rnd(int nbr_episodes);
public:
	LearningGame(int nbr_probs, double min_prob = 0, double max_prob = 1);
	~LearningGame();

	void play(int nbr_episodes);

	void cp_probs(double *probs_cp);
	void cp_stats(double *stats_cp);
	int get_nbr_stats();
	int get_nbr_probs();


};

#endif /* LEARNINGGAME_H_ */
