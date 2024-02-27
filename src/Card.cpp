/*
 * Card.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#include "Card.h"

#include <stdexcept>
#include <cassert>

Card::Card() : su(NON_SU), rnk(NON_RNK), id(NON_ID) {}

const Card Card::NONE = Card();

const std::string Card::RNK_STR[] = {"2", "3", "4", "5", "6", "7", "8", "9",
									 "X", "J", "Q", "K", "A", "-"};
const std::string Card::SU_UNC_STR[] = {"\u2660", "\u2665", "\u2663", "\u2666", "-"};
const std::string Card::SU_STR[] = {"S", "H", "C", "D", "-"};

Card::Card(Cid id)
{
#ifdef DEBUG
	if (id >= N_CARDS && id != NON_ID || id < 0)
		throw std::invalid_argument("Card::Card(id): Invalid card id.");
	if (id == NON_ID)
		throw std::invalid_argument("Card::Card(id): You can't construct NONE card this way!");
#endif
	if (id == NON_ID || id >= N_CARDS || id < 0)
	{
		su = NON_SU;
		rnk = NON_RNK;
		this->id = NON_ID;
	}
	else
	{
		su = id / N_RANKS;
		rnk = id % N_RANKS;
		this->id = id;
	}
}

Card::Card(Suit su, Rank rnk)
{
	if (su < 0 || su >= N_SUITS || rnk < two || rnk > Ace)
	{
		this->su = NON_SU;
		this->rnk = NON_RNK;
		this->id = NON_ID;
	}
	else
	{
		this->su = su;
		this->rnk = rnk;
		this->id = su * N_RANKS + rnk;
	}
}

Card &Card::set(Suit su, Rank rnk)
{
	assert(su >= 0 && su < N_SUITS);
	assert(rnk >= two && rnk <= Ace);

	this->su = su;
	this->rnk = rnk;
	this->id = su * N_RANKS + rnk;

	return *this;
}

Card::Card(std::string card_str)
{
	if (card_str.length() != 2)
	{
		su = NON_SU;
		rnk = NON_RNK;
		id = NON_ID;
		return;
	}

	int r, s;
	for (r = 0; r < N_RANKS; r++)
		if (RNK_STR[r][0] == card_str[0])
			break;
	for (s = 0; s < N_SUITS; s++)
		if (SU_STR[s][0] == card_str[1])
			break;
	if (s == NON_SU || r == NON_RNK)
	{
		su = NON_SU;
		rnk = NON_RNK;
		id = NON_ID;
	}
	else
	{
		su = (s);
		rnk = (r);
		id = (su)*N_RANKS + (rnk);
	}
}

std::string Card::to_string() const
{
	return RNK_STR[rnk] + SU_STR[su];
}

std::string Card::to_unc_str() const
{
	return RNK_STR[rnk] + SU_UNC_STR[su];
}

#define REDonWHT "\033[31;107m"
#define BLKonWHT "\033[30;107m"
#define CLreset "\033[0m"

std::string Card::to_color_unc_str() const
{
	if (id == NON_ID)
		return "--";
	return ((su % 2) ? REDonWHT : BLKonWHT) + RNK_STR[rnk] + SU_UNC_STR[su] + CLreset;
}

bool Card::operator==(const Card &rhs) const
{
	return id == rhs.id;
}

bool Card::operator!=(const Card &rhs) const
{
	return id != rhs.id;
}

int Card::cmp(const Card &rhs, Suit trump)
{
	return cmp(*this, rhs, this->su, trump);
}

int Card::cmp(const Card &cl, const Card &cr, Suit led, Suit trump)
{
	if (cl.su == cr.su)
	{
		if (cl.rnk > cr.rnk)
			return 1;
		else if (cl.rnk == cr.rnk)
			return 0;
		return -1;
	}
	if (cl != NONE && cr == NONE)
		return 1;
	if (cl == NONE && cr != NONE)
		return -1;
	if (cl.su == trump)
		return 1;
	if (cl.su == led && cr.su != trump)
		return 1;
	if (cl.su != led && cl.su != trump && cr.su != led && cr.su != trump)
		return 0;
	return -1;
}
