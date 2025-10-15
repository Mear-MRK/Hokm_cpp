/*
 * InteractiveAgent.cpp
 *
 *  Created on: Jun 1, 2023
 *      Author: mear
 */

#include "InteractiveAgent.h"

#include <iostream>
#include <algorithm>

InteractiveAgent::InteractiveAgent(bool show_hand) :
Agent(),
show_hand(show_hand)
{
	name = "IN_" + std::to_string(player_id);
}

void InteractiveAgent::init_game()
{
	name = "Pl_" + std::to_string(player_id);
	output("/ALRGreetings " + name + ", you're part of team " + std::to_string(player_id % 2));
	std::string ag_name = input("Enter your name: ");
	if (ag_name.size() > 0)
		name = (ag_name.size() > 16) ? ag_name.substr(0, 16) : ag_name;
	output("/ALRWait...");
}

void InteractiveAgent::init_round(const Hand &hand)
{
	this->hand = hand;
	output("/HND"+ hand.to_string());
}

Card InteractiveAgent::act(const State &state, const History &)
{

	if (show_hand)
	{
		output("/HND" + hand.to_string());
	}

	Card inp_card;

	do
	{
		std::string inp_card_str = input("Enter your card: ");
		std::transform(inp_card_str.begin(), inp_card_str.end(),
					   inp_card_str.begin(), ::toupper);
		inp_card = Card(inp_card_str);

		if (inp_card == Card::NONE)
		{
			output("/ALRWrong string!");
			continue;
		}
		if (state.led != Card::NON_SU && hand.len[state.led] != 0 && inp_card.su != state.led)
		{
			output("/ALRYou should play the led suit: " + Card::SU_STR[state.led]);
			continue;
		}
		if (!hand.remove(inp_card))
		{
			output("/ALRYou don't have this card!");
			continue;
		}

		break;
	} while (true);
	output("/ALR");
	return inp_card;
}

Suit InteractiveAgent::call_trump(const CardStack &first_5cards)
{
	Hand f5 = first_5cards.to_Hand();
	output("/ALRIt's your turn to call the trump.");
	output("/HND" + f5.to_string());

	std::string inp_trump_str = "";
	Suit trump = 0;
	bool cnd = true;
	do
	{
		std::string prompt = "Call the trump suit: ";
		inp_trump_str = input(prompt);
		std::transform(inp_trump_str.begin(), inp_trump_str.end(), inp_trump_str.begin(), ::toupper);
		if (inp_trump_str.size() != 0)
		{
			for (trump = 0; trump < Card::N_SUITS; trump++)
			{
				if (inp_trump_str == Card::SU_STR[trump])
				{
					cnd = false;
					break;
				}
			}
		}
		if (cnd)
			output("/ALRTry again.\n");
	} while (cnd);
	output("/ALR");
	return trump;
}

void InteractiveAgent::trick_result(const State &,
									const std::array<int, Hokm::N_TEAMS> &)
{
	// int trick_taker = state.turn;
}

void InteractiveAgent::info(const std::string &info_str)
{
	output(info_str);
}