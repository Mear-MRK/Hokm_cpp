#pragma once

#include <string>
#include <cstdint>

using Suit = int;
using Rank = int;
using Cid = int;

class Card
{

public:
	Suit su;
	Rank rnk;
	Cid id;

	Card();
	Card(Cid id);
	Card(Suit su, Rank rnk);
	Card(std::string str);

	Card& set(Suit su, Rank rnk);

	static int cmp(const Card &cl, const Card &cr, Suit led, Suit trump);
	int cmp(const Card &rhs, Suit trump);

	std::string to_string() const;
	std::string to_unc_str() const;
	std::string to_color_unc_str() const;

	bool operator==(const Card &oth) const;
	bool operator!=(const Card &oth) const;

	enum SUIT
	{
		Spade = 0,
		Heart,
		Club,
		Diamond,
		NON_SU
	};
	enum RANK
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

	static const int N_CARDS = 52;
	static const int N_SUITS = 4;
	static const int N_RANKS = N_CARDS / N_SUITS;

	static const Cid NON_ID = N_CARDS + N_RANKS;
	//	static const Suit NON_SU = N_SUITS;
	//	static const Rank NON_RNK = N_RANKS;

	static const std::string RNK_STR[];
	static const std::string SU_STR[];
	static const std::string SU_UNC_STR[];
};
