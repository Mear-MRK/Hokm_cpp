/*
 * Card.h
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#ifndef CARD_H_
#define CARD_H_

#include <string>
#include <cstdint>

using Suit = std::uint8_t;
using Rank = std::uint8_t;
using CID = std::uint8_t;

class Card
{

public:
	Suit su;
	Rank rnk;
	CID id;

	Card();
	Card(CID id);
	Card(Suit su, Rank rnk);
	Card(std::string str);

	void set(Suit su, Rank rnk);

	static int cmp(const Card &cl, const Card &cr, Suit led, Suit trump);
	int cmp(const Card &rhs, Suit trump);

	std::string to_string() const;
	std::string to_unc_str() const;
	std::string to_clr_unc_str() const;

	bool operator==(const Card &oth) const;
	bool operator!=(const Card &oth) const;

	enum SUIT : uint8_t
	{
		Spade = 0,
		Heart,
		Club,
		Diamond,
		NON_SU
	};
	enum RANK : uint8_t
	{
		two = 0,
		three,
		four,
		five,
		six,
		seven,
		eight,
		nine,
		ten,
		Jack,
		Queen,
		King,
		Ace,
		NON_RNK
	};

	static const Card NONE;

	static const std::uint8_t N_CARDS = 52;
	static const std::uint8_t N_SUITS = 4;
	static const std::uint8_t N_RANKS = N_CARDS / N_SUITS;

	static const CID NON_ID = N_CARDS + N_RANKS;
	//	static const Suit NON_SU = N_SUITS;
	//	static const Rank NON_RNK = N_RANKS;

	static const std::string RNK_STR[];
	static const std::string SU_STR[];
	static const std::string SU_UNC_STR[];
};

#endif /* CARD_H_ */
