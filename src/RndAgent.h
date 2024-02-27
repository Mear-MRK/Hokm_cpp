/*
 * Agents.h
 *
 *  Created on: May 23, 2023
 *      Author: mear
 */

#ifndef RNDAGENT_H_
#define RNDAGENT_H_

#include "GameConfig.h"
#include "Agent.h"
#include <random>

class RndAgent : public Agent {
private:
	std::mt19937 mt_rnd_gen;
public:
	RndAgent();
	Card act(const State&, const History&) override;
	Suit call_trump(const CardStack&) override;
};

#endif /* RNDAGENT_H_ */
