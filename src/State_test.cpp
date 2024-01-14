/*
 * State_test.cpp
 *
 *  Created on: May 22, 2023
 *      Author: mear
 */
#include "State.h"

#include <iostream>

void State_test() {
	State s;
	s.led = Card::Diamond;
	s.ord = 1;
	s.table[0] = Card(0, 0);
	s.table[1] = Card(1, 1);
	s.table[2] = Card(2, 2);
	s.table[3] = Card(3, 3);

	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		std::cout << " " << s.table[pl].to_string();
	std::cout << std::endl;
	s.reset();
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		std::cout << " " << s.table[pl].to_string();
	std::cout << std::endl;

}
