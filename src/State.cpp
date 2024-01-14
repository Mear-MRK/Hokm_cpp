/*
 * State.cpp
 *
 *  Created on: May 22, 2023
 *      Author: mear
 */

#include "GameConfig.h"
#include "State.h"

#include <algorithm>


State::State() : trick_id(255), turn(255), ord(255), trump(Card::NON_SU), led(Card::NON_SU), score{0} {
}

void State::reset() {
	ord = 255;
	led = Card::NON_SU;
	for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		table[pl] = Card::NONE;
	std::fill(score, score + Hokm::N_TEAMS, 0);
}

std::string State::to_string() {
	std::string out = "";
	out += "trick_id: " + std::to_string((int)trick_id) + ", turn: " + std::to_string((int)turn) + ", ord: " + std::to_string((int)ord);
	out += "\nled: " + Card::SU_STR[led] + ", trump: " + Card::SU_STR[trump];
	out += "\nTable:\n  " + table[0].to_string() + "\n" + table[1].to_string()
				+ "  " + table[3].to_string() + "\n  " + table[2].to_string();
	out += "\nscores " + std::to_string(score[0]) + " : " + std::to_string(score[1])
				+ "\n";
	return out;
}
