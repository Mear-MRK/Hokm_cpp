/*
 * History_test.cpp
 *
 *  Created on: May 23, 2023
 *      Author: mear
 */

#include "History.h"

#include <iostream>

void History_test() {
	History h;

	for(int pl = 0; pl < 4; pl++)
		std::cout << h.played_cards[pl].to_string() << std::endl;

	h.played_cards[0].append(Card(0,0));
	h.played_cards[2].append(Card(2, 2));

	for(int pl = 0; pl < 4; pl++)
		std::cout << h.played_cards[pl].to_string() << std::endl;

}

