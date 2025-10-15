/*
 * LearningGame.cpp
 *
 *  Created on: May 31, 2023
 *      Author: mear
 */

#include "LearningGame.h"

#include <cstring>

#include "SoundAgent.h"
#include "RndAgent.h"
#include "utils.h"

LearningGame::LearningGame(int nbr_probs, double min_prob, double max_prob) :
	nbr_probs(nbr_probs),
	nbr_stats{0},
	stats{nullptr}
{
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		agent[pl] = new SoundAgent();
	// agent[0] = new SoundAgent();
	// agent[1] = new RndAgent();
	// agent[2] = new SoundAgent();
	// agent[3] = new RndAgent();

	round = std::unique_ptr<GameRound>(new GameRound(agent));

	probs = new double[nbr_probs];

	for (int i = 0; i < nbr_probs; i++)
	{
		probs[i] = min_prob + (double)i * (max_prob - min_prob) / (nbr_probs - 1);
	}
}

LearningGame::~LearningGame()
{
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		delete agent[pl];
	delete[] probs;
	delete[] stats;
}

void LearningGame::tweak_floor_trump_probs(int nbr_episodes)
{

	nbr_stats = nbr_probs * nbr_probs;
	stats = new double[nbr_stats];
	std::fill((stats), (stats + nbr_stats), 0);

	for (int i1 = 0; i1 < nbr_probs - 1; i1++)
	{
		for (int j1 = i1 + 1; j1 < nbr_probs; j1++)
		{
			((SoundAgent *)agent[0])->set_probs(probs[i1], probs[j1]);
			((SoundAgent *)agent[2])->set_probs(probs[i1], probs[j1]);
			for (int i2 = 0; i2 < nbr_probs - 1; i2++)
			{
				for (int j2 = i2 + 1; j2 < nbr_probs; j2++)
				{
					((SoundAgent *)agent[1])->set_probs(probs[i2], probs[j2]);
					((SoundAgent *)agent[3])->set_probs(probs[i2], probs[j2]);

					for (int e = 0; e < nbr_episodes; e++)
					{
						int nbr_team_0_wins = 0;
						for (int r = 1; r <= 2 * Hokm::WIN_SCORE; r++)
						{
							round->reset();
							round->deal_n_init();
							round->trump_call();
							int winner_team = round->play();

							if (winner_team == 0)
							{
								stats[i1 * nbr_probs + j1]++;
								nbr_team_0_wins++;
							}
							else
								stats[i2 * nbr_probs + j2]++;
						}
						for (auto ag : agent)
							ag->fin_game();
						LOG(
							"e: " << e << ", winner team: " << ((nbr_team_0_wins > 6) ? 1 : 2));
						round->winner_team = -1;
					}
				}
			}
		}
	}
	double mx_cnt = 0;
	double op_max_p = 1;
	double op_min_p = 0;
	for (int i = 0; i < nbr_probs - 1; i++)
	{
		for (int j = i + 1; j < nbr_probs; j++)
		{
			double cnt = stats[nbr_probs * i + j];
			if (cnt > mx_cnt)
			{
				mx_cnt = cnt;
				op_min_p = probs[i];
				op_max_p = probs[j];
			}
			std::cout << cnt << " ";
		}
		std::cout << std::endl;
	}

	std::cout << "Optimal floor_p: " << op_min_p << ", trump_cap_p: " << op_max_p << std::endl;
}

void LearningGame::tweak_trump_prob_cap(int nbr_episodes)
{

	nbr_stats = nbr_probs;
	stats = new double[nbr_stats];
	std::fill((stats), (stats + nbr_stats), 0);

	for (int i = 0; i < nbr_probs; i++)
	{
		((SoundAgent *)agent[0])->set_probs(0, probs[i]);
		((SoundAgent *)agent[2])->set_probs(0, probs[i]);
		for (int j = 0; j < nbr_probs; j++)
		{
			if (j == i)
				continue;
			((SoundAgent *)agent[1])->set_probs(0, probs[j]);
			((SoundAgent *)agent[3])->set_probs(0, probs[j]);

			for (int e = 0; e < nbr_episodes; e++)
			{
				int nbr_team_0_wins = 0;
				for (int r = 1; r <= 2 * Hokm::WIN_SCORE; r++)
				{
					round->reset();
					round->deal_n_init();
					round->trump_call();
					int winner_team = round->play();

					if (winner_team == 0)
					{
						stats[i]++;
						nbr_team_0_wins++;
					}
					else
						stats[j]++;
				}
				for (auto ag : agent)
					ag->fin_game();
				LOG(
					"e: " << e << ", winner team: " << ((nbr_team_0_wins > 6) ? 1 : 2));
				round->winner_team = -1;
			}
		}
	}
}

void LearningGame::tweak_floor_prob(int nbr_episodes)
{

	nbr_stats = nbr_probs;
	stats = new double[nbr_stats];
	std::fill((stats), (stats + nbr_stats), 0);

	for (int i = 0; i < nbr_probs; i++)
	{
		((SoundAgent *)agent[0])->set_probs(probs[i]);
		((SoundAgent *)agent[2])->set_probs(probs[i]);
		for (int j = 0; j < nbr_probs; j++)
		{
			if (j == i)
				continue;
			((SoundAgent *)agent[1])->set_probs(probs[j]);
			((SoundAgent *)agent[3])->set_probs(probs[j]);

			for (int e = 0; e < nbr_episodes; e++)
			{
				for (int r = 1; r <= 2 * Hokm::WIN_SCORE; r++)
				{
					round->reset();
					round->deal_n_init();
					round->trump_call();
					int winner_team = round->play();

					if (winner_team == 0)
						stats[i]++;
					else
						stats[j]++;
				}
				for (auto ag : agent)
					ag->fin_game();
				round->winner_team = -1;
			}
		}
	}

	for (int i = 0; i < nbr_probs; i++)
	{
		std::cout << probs[i] << " ";
	}
	std::cout << std::endl;

	double mx_cnt = 0;
	double prb_max = 1;
	for (int i = 0; i < nbr_stats; i++)
	{
		double cnt = stats[i];
		if (cnt > mx_cnt)
		{
			mx_cnt = cnt;
			prb_max = probs[i];
		}
		std::cout << cnt << " ";
	}
	std::cout << "\nOptim floor prob: " << prb_max << std::endl;
}

void LearningGame::tweak_floor_prob_vs_rnd(int nbr_episodes)
{
	nbr_stats = nbr_probs;
	stats = new double[nbr_stats];
	int rnd_wins[nbr_stats] = {0};
	std::fill(stats, stats + nbr_stats, 0);

	for (int i = 0; i < nbr_probs; i++)
	{
		((SoundAgent *)agent[0])->set_probs(probs[i]);
		((SoundAgent *)agent[2])->set_probs(probs[i]);

		for (int e = 0; e < nbr_episodes; e++)
		{
			for (int r = 1; r <= 2 * Hokm::WIN_SCORE; r++)
			{
				round->reset();
				round->deal_n_init();
				round->trump_call();
				int winner_team = round->play();

				if (winner_team == 0)
					stats[i]++;
				else
					rnd_wins[i]++;
			}
			for (auto ag : agent)
				ag->fin_game();
			round->winner_team = -1;
		}
	}

	std::cout << "Probs:\n";
	for (int i = 0; i < nbr_probs; i++)
	{
		std::cout << probs[i] << " ";
	}
	std::cout << std::endl;
	std::cout << "Ratios:\n";
	double sum = 0;
	double mx_cnt = 0;
	double prb_max = 1;
	double r_m = 0;
	for (int i = 0; i < nbr_stats; i++)
	{
		double cnt = stats[i];
		double r_cnt = rnd_wins[i];
		double r = cnt / r_cnt;
		if (cnt > mx_cnt)
		{
			mx_cnt = cnt;
			prb_max = probs[i];
			r_m = r;
		}
		sum += r;
		std::cout << r << " ";
	}
	std::cout << std::endl;
	std::cout << "Optim. floor prob: " << prb_max << ", max ratio: " << r_m << "\n";
	std::cout << "Avg. ratio: " << sum / nbr_probs << std::endl;
}

void LearningGame::play(int nbr_episodes)
{
	tweak_floor_prob(nbr_episodes);
	// tweak_floor_prob_vs_rnd(nbr_episodes);
}

void LearningGame::cp_probs(double *probs_cp)
{
	std::copy((probs), (probs + nbr_probs), probs_cp);
}

void LearningGame::cp_stats(double *stats_cp)
{
	std::copy((stats), (stats + nbr_stats), stats_cp);
}

int LearningGame::get_nbr_probs()
{
	return nbr_probs;
}

int LearningGame::get_nbr_stats()
{
	return nbr_stats;
}
