/*
 * History.h
 *
 *  Created on: May 23, 2023
 *      Author: mear
 */

#ifndef HISTORY_H_
#define HISTORY_H_

#include "GameConfig.h"
#include "CardStack.h"

class History
{
public:
	CardStack played_cards[Hokm::N_PLAYERS];

	History();

	void reset();
};

#endif /* HISTORY_H_ */
