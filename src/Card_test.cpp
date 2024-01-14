/*
 * Card_test.cpp
 *
 *  Created on: May 23, 2023
 *      Author: mear
 */
#include "Card.h"

#include <iostream>

void Card_test(){
	Card c;
	if (c == Card::NONE)
		std::cout << "c is none." << std::endl;

	Card d = Card::NONE;
	Suit led = Card::NON_SU;
	Suit trump = Card::NON_SU;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

	led = Card::Diamond;
	trump = Card::Heart;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

	led = Card::Diamond;
	trump = Card::NON_SU;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

	led = Card::NON_SU;
	trump = Card::Heart;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

	d = Card(Card::Diamond, Card::three);
	c = Card(Card::Diamond, Card::two);
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

	c = Card(Card::Heart, Card::Ace);
	led = Card::Diamond;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

	led = Card::Heart;
	trump = Card::Diamond;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;
	led = Card::NON_SU;
	trump = Card::NON_SU;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;
	led = Card::Club;
	trump = Card::Spade;
	std::cout << "d: " << d.to_string() << ", c: " << c.to_string()
			  << ", led: " << Card::SU_STR[(led)] << ", trump: " << Card::SU_STR[(trump)]
			  << " ; cmp(d, c) : " << Card::cmp(d, c, led, trump) << ", "
			  << " ; cmp(c, d) : " << Card::cmp(c, d, led, trump) << std::endl;

}



