/*
 * test_Hand.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */
#include "Hand.h"

#include <iostream>
#include "Card.h"

void Hand_test() {
	Hand h0;
	CID cids[] = { 0, 1, 2, 3, 3, 13, 26, 39, 51, 0 };
	Card cards[10];
	for(int i = 0; i < 10; i++)
		cards[i] = Card(cids[i]);

	Hand h1(cards, 10);
	std::cout << "h0: " << h0.to_string() << std::endl;
	std::cout << "h1: " << h1.to_string() << ":\n"
			<< h1.to_su_string() << std::endl;
	std::cout << "h1.bin64: " << h1.bin64 << std::endl;
	h0.add(Card(15));
	h0.add(Card(15));
	h0.add(Card(13));
	h0.remove(Card(15));
	h0.add({Card::Spade, Card::four});
	h1.pop(Card::Heart, 0);
	std::cout << "h0: " << h0.to_string() << std::endl;
	std::cout << "h1: " << h1.to_string() << std::endl;
	Hand h2(h1.bin64);
	std::cout << "h2(h1.bin64): " << h2.to_string() << std::endl << h2.to_su_string() << std::endl;
	std::cout << "h1.bin64: " << h1.bin64 << std::endl;
	std::cout << "h2.bin64: " << h2.bin64 << std::endl;

	std::cout << "cmpl h1: " << h1.cmpl().to_string() << std::endl;
	std::cout << "h0 uni h1: " << h0.uni(h1).to_string() << std::endl;
	std::cout << "h0 int h1: " << h0.inter(h1).to_string() << std::endl;

}



