/*
 * CardArray_test.cpp
 *
 *  Created on: May 22, 2023
 *      Author: mear
 */

#include "CardStack.h"

#include <iostream>

void CardStack_test() {
	CardStack cs;
	std::cout << "empty: " << cs.to_string() << std::endl;

//	cs.append(Card::NONE);
	cs.append(Card(51));
	cs.append(Card(Card::Diamond, Card::three));
	cs.append(Card(Card::Spade, Card::Jack));
	//cs.append(Card(Card::NON_SU, Card::King));
	cs.append(Card("XH"));
	for(int i = 0; i < 13; i++)
		cs.append(Card(i));

	std::cout << cs.to_string() << std::endl;
	std::cout << cs.at(-2).to_string() << std::endl;
	std::cout << cs.pop().to_string() << std::endl;
	std::cout << cs.to_string() << std::endl;
	std::cout << cs.to_Hand().to_su_string() << std::endl;
}
