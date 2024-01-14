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

#include "CardProb.h"
#include "utils.h"

Hand::Hand() : bin64(0), nbr_cards(0)
{
	memset(len, 0, sizeof(len));
}

const Hand Hand::EMPTY = Hand();

Hand::Hand(const Card *cards, std::uint8_t nbr_cards) : bin64(0), nbr_cards(0)
{
	memset(len, 0, sizeof(len));
	add(cards, nbr_cards);
}

Hand::Hand(std::uint64_t bin64) : bin64(0), nbr_cards(0)
{
	memset(len, 0, sizeof(len));
	uint64_t mask = ~0ull << (64 - Card::N_CARDS) >> (64 - Card::N_CARDS);
	bin64 &= mask;

	for (CID id = 0; id < Card::N_CARDS; id++)
		if (bin64 & (1ull << id))
		{
			Card c = Card(id);
			add(c);
		}
}

void Hand::add(const Card &c)
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
}

Card Hand::pop(Suit su, std::uint8_t ord)
{
#ifdef DEBUG
	if (su >= Card::N_SUITS || ord >= len[su])
		throw std::invalid_argument("Hand::pop : Out of bound index.");
#endif
	move<Rank>(cards[su], len[su], ord + 1, -1);
	len[su]--;
	Card out = Card(su, cards[su][ord]);
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

void Hand::add(const Card *cards, std::uint8_t nbr_cards)
{
#ifdef DEBUG
	if (cards == nullptr)
		throw std::invalid_argument("Hand::add : null cards!");
	if (nbr_cards < 1)
		throw std::invalid_argument(
			"Hand::add : Num. of cards must be non-zero positive.");
#endif
	for (std::uint8_t i = 0; i < nbr_cards; i++)
		add(cards[i]);
}

void Hand::add(const Hand &hand)
{
	for (Suit s = 0; s < Card::N_SUITS; s++)
		for (uint8_t i = 0; i < hand.len[s]; i++)
			add({s, hand.cards[s][i]});
}

bool Hand::remove(const Card &c)
{
	// #ifdef DEBUG
	//	if (c == Card::NONE)
	//		throw std::invalid_argument("Hand::remove : NONE.");
	// #endif
	std::uint8_t i = bisect<Rank>(cards[c.su], len[c.su], c.rnk);
	if (i < 1 || cards[c.su][i - 1] != c.rnk)
		return false;
	move<Rank>(cards[c.su], len[c.su], i, -1);
	bin64 &= ~(1ull << c.id);
	len[c.su]--;
	nbr_cards--;
	return true;
}

std::string Hand::to_string()
{
	std::string out = "";
	for (std::uint8_t s = 0; s < Card::N_SUITS; s++)
	{
		for (std::uint8_t i = 0; i < len[s]; i++)
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

std::string Hand::to_su_string()
{
	std::string out = "\n";
	Card c;
	for (std::uint8_t s = 0; s < Card::N_SUITS; s++)
	{
		out += Card::SU_STR[s] + ": ";
		for (std::uint8_t i = 0; i < len[s]; i++)
		{
			c.set(s, cards[s][i]);
			out += c.to_string(); // Card::RNK_STR[cards[s][i]];
		}
		out += "\n"; //(s != (Card::N_SUITS - 1)) ? "\n" : "";
	}
	return out;
}

void Hand::reset()
{
	memset(len, 0, sizeof(len));
	bin64 = 0;
	nbr_cards = 0;
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
	Hand out = Hand::EMPTY;
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

Hand Hand::gt(const Card &card, Suit led, Suit trump) const
{
	Hand out = Hand::EMPTY;
	Card c;
	for (Suit s = 0; s < Card::N_SUITS; s++)
	{
		for (std::uint8_t i = 0; i < len[s]; i++)
		{
			c.set(s, cards[s][i]);
			if (Card::cmp(card, c, led, trump) > 0)
				out.add(c);
		}
	}
	return out;
}

Card Hand::min_mil_trl(Suit led, Suit trump, const uint8_t *len_arr) const
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

// Hand::Hand(Suit su) {
// #ifdef DEBUG
//	if (su >= Card::N_SUITS)
//		throw std::invalid_argument("Hand(su): invalid suit.");
// #endif
//	bin64 = 0x1FFFull << su * Card::N_RANKS;
//	memset(len, 0, sizeof(len));
//	len[su] = Card::N_RANKS;
//	for(Rank r = 0; r < Card::N_RANKS; r++)
//		cards[su][r] = r;
// }

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

void Hand::discard(Suit su)
{
#ifdef DEBUG
	if (su >= Card::N_SUITS)
		throw std::invalid_argument(
			"Hand::discard(su): invalid suit: " + std::to_string((int)su));
#endif
	nbr_cards -= len[su];
	len[su] = 0;
	bin64 &= ~(0x1FFFull << su * Card::N_RANKS);
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
		throw std::invalid_argument("Hand::is_in : NONE card.");
#endif
	return sorted_is_in<Rank>(cards[c.su], len[c.su], c.rnk);
}

static inline double chs_prob(int n, int n_ex, int k)
{
	if (k > Card::N_CARDS || n_ex > Card::N_CARDS || n > Card::N_CARDS)
	{
		std::cerr << "k: " << k << ", n_ex: " << n_ex << ", n: " << n << std::endl;
		return 0;
		//		throw std::invalid_argument("chs_prob : args. out of bound.");
	}
	if (n < n_ex + k)
		return 0;
	double prb = 1;
	for (int i = 0; i < k; i++)
		prb *= static_cast<double>(n - n_ex - i) / (n - i);
	return prb;
}

double Hand::prob_lt(const Card &card, Suit led, Suit trump, int k) const
{
	if (card == Card::NONE)
		return 0;
	if (led == Card::NON_SU)
		led = card.su;
	if (card.su != led)
		if (card.su == trump)
		{
			int i = bisect<Rank>(cards[trump], len[trump], card.rnk);
			return 1 - chs_prob(nbr_cards, len[led], k) + chs_prob(nbr_cards, len[led] + len[trump] - i, k);
		}
		else
		{
			return 0;
		}
	if (led == trump || trump == Card::NON_SU)
	{
		int i = bisect<Rank>(cards[led], len[led], card.rnk);
		return chs_prob(nbr_cards, len[led] - i, k);
	}
	int i = bisect<Rank>(cards[led], len[led], card.rnk);
	return chs_prob(nbr_cards, len[led] - i, k) - chs_prob(nbr_cards, len[led], k) + chs_prob(nbr_cards, len[led] + len[trump], k);
}

Hand Hand::gt_prob(const Hand &rhs, Suit led, Suit trump, int k,
				   double prob_floor, double trump_prob_cap, double prob_ceiling) const
{
	if (!nbr_cards)
		return Hand::EMPTY;
	if (!rhs.nbr_cards)
		return *this;
	Hand out;
	if (led != Card::NON_SU)
		if (len[led] != 0)
		{
			for (uint8_t i = 0; i < len[led]; i++)
			{
				Card c = Card(led, cards[led][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				LOG("card: " << c.to_string() << ", prob: " << prb);
				if (prb > prob_floor && prb <= prob_ceiling)
					out.add(c);
			}
			return out;
		}
		else
		{
			for (uint8_t i = 0; i < len[trump]; i++)
			{
				Card c = Card(trump, cards[trump][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				LOG("card: " << c.to_string() << ", prob: " << prb);
				if (prb > prob_floor && prb <= trump_prob_cap)
					out.add(c);
			}
			return out;
		}
	// no led
	for (Suit s = 0; s < Card::N_SUITS; s++)
		if (s != trump)
			for (uint8_t i = 0; i < len[s]; i++)
			{
				Card c = Card(s, cards[s][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				LOG("card: " << c.to_string() << ", prob: " << prb);
				if (prb > prob_floor && prb <= prob_ceiling)
					out.add(c);
			}
	if (out.nbr_cards)
		return out;

	for (uint8_t i = 0; i < len[trump]; i++)
	{
		Card c = Card(trump, cards[trump][i]);
		double prb = rhs.prob_lt(c, led, trump, k);
		LOG("card: " << c.to_string() << ", prob: " << prb);
		if (prb > prob_floor && prb <= trump_prob_cap)
			out.add(c);
	}
	return out;
}

Card Hand::max_prob_trl(const Hand &rhs, Suit led, Suit trump, int k,
						double prob_cap, double min_prob, double trump_prb_cap) const
{
	Card max_c;
	double max_prb = min_prob;
	if (led != Card::NON_SU)
	{
		if (len[led] != 0)
		{
			for (uint8_t i = 0; i < len[led]; i++)
			{
				Card c = Card(led, cards[led][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				if (prb > max_prb && prb <= prob_cap)
				{
					max_prb = prb;
					max_c = c;
				}
			}
			LOG(
				"led, max_prob: " << max_prb << ", card: " << max_c.to_string());
			return max_c;
		}
		else if (trump != Card::NON_SU)
		{
			for (int i = 0; i < len[trump]; i++)
			{
				Card c = Card(trump, cards[trump][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				if (prb > max_prb && prb <= trump_prb_cap)
				{
					max_prb = prb;
					max_c = c;
				}
			}
			LOG(
				"trump, max_prob: " << max_prb << ", card: " << max_c.to_string());
			return max_c;
		}
	}
	for (Suit s = 0; s < Card::N_SUITS; s++)
		if (s != trump)
		{
			for (uint8_t i = 0; i < len[s]; i++)
			{
				Card c = Card(s, cards[s][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				if (prb > max_prb && prb <= prob_cap)
				{
					max_prb = prb;
					max_c = c;
				}
			}
		}
	LOG(
		"non-led non-trump, max_prob: " << max_prb << ", card: " << max_c.to_string());
	if (max_c != Card::NONE)
	{
		return max_c;
	}
	if (trump != Card::NON_SU)
	{
		for (int i = 0; i < len[trump]; i++)
		{
			Card c = Card(trump, cards[trump][i]);
			double prb = rhs.prob_lt(c, led, trump, k);
			if (prb > max_prb && prb <= trump_prb_cap)
			{
				max_prb = prb;
				max_c = c;
			}
		}
		LOG(
			"non-led, trump, max_prob: " << max_prb << ", card: " << max_c.to_string());
		return max_c;
	}
	return max_c;
}

Card Hand::min_gt_prob(const Hand &rhs, Suit led, Suit trump, int k,
					   double prob_floor, double trump_prob_cap, double prob_ceiling) const
{
	if (!nbr_cards)
		return Card::NONE;

	Card out;
	if (led != Card::NON_SU)
	{
		if (len[led])
		{
			bool least = true;
			for (int i = 0; i < len[led]; i++)
			{
				Card c = Card(led, cards[led][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				LOG("led, prb: " << prb << ", c: " << c.to_string());
				if (prb > prob_floor && prb <= prob_ceiling)
					return c;
				else if (least && prb > 0)
				{
					out = c;
					least = false;
				}
			}
			return out;
		}
		else if (led != trump && len[trump])
		{
			for (int i = 0; i < len[trump]; i++)
			{
				Card c = Card(trump, cards[trump][i]);
				double prb = rhs.prob_lt(c, led, trump, k);
				LOG("trump, prb: " << prb << ", c: " << c.to_string());
				if (prb > 0)
					return c;
			}
			return out;
		}
	}
	// led is NON
	CardProb crd_prb[13];
	size_t n = 0;
	for (Suit s = 0; s < Card::N_SUITS; s++)
		if (s != trump)
			for (int i = 0; i < len[s]; i++)
			{
				Card c = Card(s, cards[s][i]);
				crd_prb[n].prob = rhs.prob_lt(c, led, trump, k);
				crd_prb[n++].card = c;
			}
	if (n)
	{
		quicksort<CardProb>(crd_prb, n);
#ifdef DEBUG
		std::cout << '\n';
		for (int i = 0; i < n; i++)
			std::cout << crd_prb[i].prob << ',' << crd_prb[i].card.to_string()
					  << " ";
		std::cout << std::endl;
#endif
		CardProb cp_fl;
		cp_fl.prob = prob_floor;
		int i = bisect<CardProb>(crd_prb, n, cp_fl);
		if (i < n && crd_prb[i].prob <= prob_ceiling)
			return crd_prb[i].card;
		else if (crd_prb[n - 1].prob > 0)
			return crd_prb[n - 1].card;
		return out;
	}
	for (int i = 0; i < len[trump]; i++)
	{
		Card c = Card(trump, cards[trump][i]);
		double prb = rhs.prob_lt(c, led, trump, k);
		if (prb > 0)
			return c;
	}

	return out;
}

bool Hand::card_is_valid(const Card &card, Suit led) const
{
	if (!this->is_in(card))
	{
		LOG("Hand::card_is_valid : card " << card.to_string() << " is not in the hand.");
		return false;
	}
	if (led == Card::NON_SU)
		return true;
	if ((card.su != led) && (len[led] != 0))
	{
		LOG("Hand::card_is_valid : card suit is not led : " << Card::SU_STR[led]);
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
