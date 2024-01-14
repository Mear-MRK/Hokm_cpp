/*
 * History.cpp
 *
 *  Created on: May 23, 2023
 *      Author: mear
 */

#include "History.h"

History::History()
{
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		played_cards[pl] = CardStack::EMPTY;
}

void History::reset()
{
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		played_cards[pl] = CardStack::EMPTY;
}
