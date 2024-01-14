/*
 * State.h
 *
 *  Created on: May 22, 2023
 *      Author: mear
 */

#ifndef STATE_H_
#define STATE_H_

#include <cstdint>
#include <string>

#include "GameConfig.h"
#include "Card.h"



class State {

public:
	std::uint8_t trick_id;
	std::uint8_t turn;
	std::uint8_t ord;
	Suit trump;
	Suit led;
	Card table[Hokm::N_PLAYERS];
	int score[Hokm::N_TEAMS];

	State();

	std::string to_string();

	void reset();

};

#endif /* STATE_H_ */
