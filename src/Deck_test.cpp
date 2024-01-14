/*
 * test_Deck.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#include "Deck.h"

#include <iostream>

#include "CardStack.h"


void Deck_test()
{
	CardStack h[4];
	Deck dk;
	for(int i = 0; i < 2; i++) {
		dk.shuffle_deal(h, 4);
		for(int j = 0; j < 4; j++)
			std::cout << h[j].to_string() << std::endl;
	std::cout << "------------------------------------------" << std::endl;
	}
}



