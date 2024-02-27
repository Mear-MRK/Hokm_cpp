/*
 * Hand.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */
#include "Hand.h"

#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <cassert>

#include "utils.h"

Hand::Hand() : bin64(0), nbr_cards(0)
{
	memset(len, 0, sizeof(len));
}

const Hand Hand::EMPTY = Hand();

Hand::Hand(const Card *cards, int nbr_cards) : bin64(0), nbr_cards(0)
{
	memset(len, 0, sizeof(len));
	add(cards, nbr_cards);
}

Hand::Hand(std::uint64_t bin64) : bin64(0), nbr_cards(0)
{
	memset(len, 0, sizeof(len));
	uint64_t mask = ~0ull << (64 - Card::N_CARDS) >> (64 - Card::N_CARDS);
	bin64 &= mask;

	for (Cid id = 0; id < Card::N_CARDS; id++)
		if (bin64 & (1ull << id))
		{
			Card c = Card(id);
			add(c);
		}
}

Hand &Hand::add(const Card &c)
{
#ifdef DEBUG
	if (c == Card::NONE)
		throw std::invalid_argument(
			"Hand::add(c) : Card::NONE can't be added.");
	if (len[c.su] == Card::N_RANKS)
	{
		print_arr(cards[c.su], len[c.su], true);
		throw std::invalid_argument(
			"Hand::add(c) : Suit array is full. : " + c.to_string());
	}
#endif
	if (sorted_unique_insert<Rank>(cards[c.su], len[c.su], c.rnk))
	{
		len[c.su]++;
		bin64 |= 1ull << c.id;
		nbr_cards++;
	}
	return *this;
}

Card Hand::pop(Suit su, int ord)
{
#ifdef DEBUG
	if (su >= Card::N_SUITS || ord >= len[su])
		throw std::invalid_argument("Hand::pop : Out of bound index.");
#endif
	Card out = Card(su, cards[su][ord]);
	move<Rank>(cards[su], len[su], ord + 1, -1);
	len[su]--;
	bin64 ^= 1ull << out.id;
	nbr_cards--;
	return out;
}

Hand Hand::pop(Suit su)
{
	Hand h = Hand();
	Card c;
	for (int i = 0; i < len[su]; i++)
	{
		c.set(su, cards[su][i]);
		h.add(c);
	}
	discard(su);
	return h;
}

Hand &Hand::add(const Card *cards, int nbr_cards)
{
#ifdef DEBUG
	if (cards == nullptr)
		throw std::invalid_argument("Hand::add : null cards!");
	if (nbr_cards < 1)
		throw std::invalid_argument(
			"Hand::add : Num. of cards must be non-zero positive.");
#endif
	for (int i = 0; i < nbr_cards; i++)
		add(cards[i]);
	return *this;
}

Hand &Hand::add(const Hand &hand)
{
	for (Suit s = 0; s < Card::N_SUITS; s++)
		for (uint8_t i = 0; i < hand.len[s]; i++)
			add({s, hand.cards[s][i]});
	return *this;
}

bool Hand::remove(const Card &c)
{
	// #ifdef DEBUG
	//	if (c == Card::NONE)
	//		throw std::invalid_argument("Hand::remove : NONE.");
	// #endif
	int i = bisect<Rank>(cards[c.su], len[c.su], c.rnk);
	if (i < 1 || cards[c.su][i - 1] != c.rnk)
		return false;
	move<Rank>(cards[c.su], len[c.su], i, -1);
	bin64 &= ~(1ull << c.id);
	len[c.su]--;
	nbr_cards--;
	return true;
}

std::string Hand::to_string() const
{
	std::string out = "";
	for (int s = 0; s < Card::N_SUITS; s++)
	{
		for (int i = 0; i < len[s]; i++)
		{
			std::string ap = " ";
			if (i + 1 == len[s])
			{
				ap = ",";
				if (s + 1 == Card::N_SUITS)
					ap = "";
			}
			out += Card(s, cards[s][i]).to_string() + ap;
		}
	}
	return out;
}

std::string Hand::to_su_string() const
{
	std::string out;
	Card c;
	for (int s = 0; s < Card::N_SUITS; s++)
	{
		out += Card::SU_STR[s] + ": ";
		for (int i = 0; i < len[s]; i++)
		{
			c.set(s, cards[s][i]);
			out += Card::RNK_STR[cards[s][i]] + " ";
		}
		out += (s +	1 != Card::N_SUITS) ? "\n" : "";
	}
	return out;
}

Hand &Hand::clear()
{
	memset(len, 0, sizeof(len));
	bin64 = 0;
	nbr_cards = 0;
	return *this;
}

Hand Hand::cmpl() const
{
	return Hand(~bin64); // it will be automatically masked (to 52 cards) in the Hand(bin64) constructor.
}

Hand Hand::uni(const Hand &rhs) const
{
	return Hand(bin64 | rhs.bin64);
}

Hand Hand::inter(const Hand &rhs) const
{
	return Hand(bin64 & rhs.bin64);
}

Hand Hand::gt_c_led(const Card &card, Suit led, Suit trump) const
{
	assert(trump != Card::NON_SU);
	led = (led != Card::NON_SU) ? led : card.su;
	if (card.su == led)
	{
		if (len[led] != 0)
		{
			uint8_t i = bisect<Rank>(cards[led], len[led], card.rnk);
			Hand out = Hand::EMPTY;
			for (uint8_t j = i; j < len[led]; j++)
				out.add(Card(led, cards[led][j]));
			return out;
		}
		else
		{
			return sub_hand(trump);
		}
	}
	// card.su != led
	if (len[led] != 0)
	{
		if (card.su != trump)
			return sub_hand(led);
		return Hand::EMPTY;
	}
	// len[led] == 0
	if (card.su != trump)
		return sub_hand(trump);
	// card.su == trump
	uint8_t i = bisect<Rank>(cards[trump], len[trump], card.rnk);
	Hand out = Hand::EMPTY;
	for (uint8_t j = i; j < len[trump]; j++)
		out.add(Card(trump, cards[trump][j]));
	return out;
}

Hand Hand::lead_gt(const Card &card, Suit led, Suit trump) const
{
	assert(trump != Card::NON_SU);
	if (led != Card::NON_SU)
	{
		if (card.su == led)
		{
			if (len[led] != 0)
			{
				uint8_t i = bisect<Rank>(cards[led], len[led], card.rnk);
				Hand out = Hand::EMPTY;
				for (uint8_t j = i; j < len[led]; j++)
					out.add(Card(led, cards[led][j]));
				return out;
			}
			else
				return sub_hand(trump);
		}
		// card.su != led
		if (len[led] != 0)
		{
			if (card.su != trump)
				return sub_hand(led);
			return Hand::EMPTY;
		}
		// len[led] == 0
		if (card.su != trump)
			return sub_hand(trump);
		// card.su == trump
		uint8_t i = bisect<Rank>(cards[trump], len[trump], card.rnk);
		Hand out = Hand::EMPTY;
		for (uint8_t j = i; j < len[trump]; j++)
			out.add(Card(trump, cards[trump][j]));
		return out;
	}
	// led == Non
	if (card.su == trump)
	{
		uint8_t i = bisect<Rank>(cards[trump], len[trump], card.rnk);
		Hand out = Hand::EMPTY;
		for (uint8_t j = i; j < len[trump]; j++)
			out.add(Card(trump, cards[trump][j]));
		return out;
	}
	// card.su != trump
	Hand out = *this;
	out.discard(card.su);
	uint8_t i = bisect<Rank>(cards[card.su], len[card.su], card.rnk);
	for (uint8_t j = i; j < len[card.su]; j++)
		out.add(Card(card.su, cards[card.su][j]));
	return out;
}

// if led is non, `this` suit gonna be lead
Hand Hand::lead_gt(const Hand &hand, Suit led, Suit trump) const
{
	Hand out;
	if (led == Card::NON_SU)
	{
		for (Suit s = 0; s < Card::N_SUITS; s++)
		{
			if (hand.len[s] == 0)
			{
				if (hand.len[trump] == 0)
					out.add(sub_hand(s));
			}
			else
			{
				uint8_t i = bisect<Rank>(cards[s], len[s], hand.cards[s][hand.len[s] - 1]);
				for (uint8_t j = i; j < len[s]; j++)
					out.add(Card(s, cards[s][j]));
			}
		}
	}
	else
	{ // led not non
		if (hand.len[led] != 0)
		{
			if (len[led] != 0)
			{
				uint8_t i = bisect<Rank>(cards[led], len[led], hand.cards[led][hand.len[led] - 1]);
				for (uint8_t j = i; j < len[led]; j++)
					out.add(Card(led, cards[led][j]));
			}
			else
			{
				out.add(sub_hand(trump));
			}
		}
		else // hand.len[led] == 0
		{
			if (hand.len[trump] == 0)
				if (len[led] != 0)
					out.add(sub_hand(led));
				else
					out.add(sub_hand(trump));
			else // hand.len[trump] != 0
				if (len[led] == 0 && len[trump] != 0)
				{
					uint8_t i = bisect<Rank>(cards[trump], len[trump], hand.cards[trump][hand.len[trump] - 1]);
					for (uint8_t j = i; j < len[trump]; j++)
						out.add(Card(trump, cards[trump][j]));
				}
		}
	}
	return out;
}

Hand Hand::gt_h_lead(const Hand &hand, Suit led, Suit trump) const
{

	Hand out;
	if (led == Card::NON_SU)
	{
		for (Suit s = 0; s < Card::N_SUITS; s++)
		{
			if (hand.len[s] != 0)
			{
				if (len[s] != 0)
				{
					uint8_t i = bisect<Rank>(cards[s], len[s], hand.cards[s][hand.len[s] - 1]);
					for (uint8_t j = i; j < len[s]; j++)
						out.add(Card(s, cards[s][j]));
				} else
				{
						out.add(sub_hand(trump));
				}
			}
		}
	}
	else
	{ // led not non
		if (hand.len[led] != 0)
		{
			if (len[led] != 0)
			{
				uint8_t i = bisect<Rank>(cards[led], len[led], hand.cards[led][hand.len[led] - 1]);
				for (uint8_t j = i; j < len[led]; j++)
					out.add(Card(led, cards[led][j]));
			}
			else
			{
				out.add(sub_hand(trump));
			}
		}
		else // hand.len[led] == 0
		{
			if (hand.len[trump] == 0)
				if (len[led] != 0)
					out.add(sub_hand(led));
				else
					out.add(sub_hand(trump));
			else // hand.len[trump] != 0
				if (len[led] == 0 && len[trump] != 0)
				{
					uint8_t i = bisect<Rank>(cards[trump], len[trump], hand.cards[trump][hand.len[trump] - 1]);
					for (uint8_t j = i; j < len[trump]; j++)
						out.add(Card(trump, cards[trump][j]));
				}
		}
	}
	return out;
}

Hand Hand::gt(const Card &card, Suit led, Suit trump) const
{
	Hand out = Hand::EMPTY;
	Card c;
	for (Suit s = 0; s < Card::N_SUITS; s++)
	{
		for (int i = 0; i < len[s]; i++)
		{
			c.set(s, cards[s][i]);
			if (Card::cmp(card, c, led, trump) > 0)
				out.add(c);
		}
	}
	return out;
}

Card Hand::min_mil_trl(Suit led, Suit trump, const int *len_arr) const
{
	if (!this->nbr_cards)
		return Card::NONE;
	if (led != Card::NON_SU && len[led] != 0)
		return Card(led, cards[led][0]);
	if (!len_arr)
		len_arr = len;

	//	uint64_t one13 = (1ull << Card::N_RANKS) - 1;
	//	uint64_t trump_msk = one13 << (trump * Card::N_RANKS);

	Suit min_s = Card::NON_SU;
	Rank min_r = Card::N_RANKS;
	uint8_t min_len = Card::N_RANKS + 1;
	for (Suit s = 0; s < Card::N_SUITS; s++)
		if (s != trump && len[s] != 0 && (cards[s][0] < min_r || cards[s][0] == min_r && len_arr[s] < min_len))
		{
			min_s = s;
			min_r = cards[s][0];
			min_len = len_arr[s];
		}
	if (min_s != Card::NON_SU)
		return Card(min_s, cards[min_s][0]);
	if (trump != Card::NON_SU && len[trump] != 0)
		return Card(trump, cards[trump][0]);
	return Card::NONE;
}

Card Hand::max_mil_trl(Suit led, Suit trump) const
{
	if (!this->nbr_cards)
		return Card::NONE;
	if (led != Card::NON_SU && len[led] != 0)
		return Card(led, cards[led][len[led - 1]]);
	Suit max_s = Card::NON_SU;
	Rank max_r = 0;
	uint8_t min_len = Card::N_RANKS + 1;
	for (Suit s = 0; s < Card::N_SUITS; s++)
		if (s != trump && len[s] != 0 && (cards[s][len[s] - 1] > max_r || cards[s][len[s] - 1] == max_r && len[s] < min_len))
		{
			max_s = s;
			max_r = cards[s][len[s] - 1];
			min_len = len[s];
		}
	if (max_s != Card::NON_SU)
		return Card(max_s, cards[max_s][len[max_s] - 1]);
	if (trump != Card::NON_SU && len[trump] != 0)
		return Card(trump, cards[trump][len[trump] - 1]);
	return Card::NONE;
}

Hand &Hand::discard(Suit su)
{
#ifdef DEBUG
	if (su >= Card::N_SUITS)
		throw std::invalid_argument(
			"Hand::discard(su): invalid suit: " + std::to_string((int)su));
#endif
	nbr_cards -= len[su];
	len[su] = 0;
	bin64 &= ~(0x1FFFull << su * Card::N_RANKS);

	return *this;
}

Hand Hand::sub_hand(Suit su) const
{
#ifdef DEBUG
	if (su >= Card::N_SUITS)
		throw std::invalid_argument("Hand::sub_hand: invalid suit.");
#endif
	uint64_t su_bin64 = bin64 & (0x1FFFull << su * Card::N_RANKS);
	return Hand(su_bin64);
}

Hand Hand::sub_hand_sup_rank(Suit su, Rank rnk) const
{
#ifdef DEBUG
	if (su >= Card::N_SUITS)
		throw std::invalid_argument("Hand::sub_hand : invalid suit.");
	if (rnk >= Card::N_RANKS)
		throw std::invalid_argument("Hand::sub_hand : invalid rank.");
#endif
	uint64_t mask = ((1ull << (Card::N_RANKS - rnk)) - 1)
					<< (su * Card::N_RANKS + rnk);
	uint64_t su_bin64 = bin64 & mask;
	return Hand(su_bin64);
}

bool Hand::is_in(const Card &c) const
{
#ifdef DEBUG
	if (c == Card::NONE)
		throw std::invalid_argument("Hand::is_in: NONE card.");
#endif
	return sorted_is_in<Rank>(cards[c.su], len[c.su], c.rnk);
}

bool Hand::card_is_valid_move(const Card &card, Suit led) const
{
	if (!this->is_in(card))
	{
		LOG("Hand::card_is_valid_move: card " << card.to_string() << " is not in the hand.");
		return false;
	}
	if (led == Card::NON_SU)
		return true;
	if ((card.su != led) && (len[led] != 0))
	{
		LOG("Hand::card_is_valid_move: card suit is not led : " << Card::SU_STR[led]);
		return false;
	}
	return true;
}

int Hand::diff_between(const Card &c_low, const Card &c_hi, Suit trump) const
{
	if (c_low.su != c_hi.su)
	{
		if (c_low.su == trump)
			return -Card::N_CARDS;
		else if (c_hi.su == trump)
			return Card::N_CARDS;
		return c_hi.rnk - c_low.rnk;
	}

	if (len[c_low.su] == 0)
		return 0;
	int i_low = bisect<Rank>(cards[c_low.su], len[c_low.su], c_low.rnk);
	int i_hi = bisect<Rank>(cards[c_hi.su], len[c_hi.su], c_hi.rnk);
	return i_hi - i_low;
}
