/*
 * Hand.h
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#ifndef HAND_H_
#define HAND_H_

#include <string>
#include <cstdint>

#include "Card.h"

class Hand {

public:

	Rank cards[Card::N_SUITS][Card::N_RANKS];
	std::uint8_t len[Card::N_SUITS];
	std::uint64_t bin64;
	std::uint8_t nbr_cards;

	Hand();
	Hand(const Card *cards, std::uint8_t nbr_cards);
	Hand(std::uint64_t bin64);
//	Hand(Suit su);

	void add(const Card &c);
	void add(const Card *cards, std::uint8_t nbr_cards);
	void add(const Hand &);

	Card pop(Suit su, std::uint8_t ind);
	Hand pop(Suit su);

	bool remove(const Card &c);
	void discard(Suit);

	bool is_in(const Card &c) const;

	Hand sub_hand(Suit su) const;
	Hand sub_hand_sup_rank(Suit su, Rank rnk) const;

	std::string to_string();
	std::string to_su_string();

	void reset();

	Hand cmpl() const;
	Hand uni(const Hand&) const;
	Hand inter(const Hand&) const;

	Hand lead_gt(const Card&, Suit led, Suit trump) const;
	Hand gt_c_led(const Card &card, Suit led, Suit trump) const;

	Hand lead_gt(const Hand&, Suit led, Suit trump) const;
	Hand gt(const Card&, Suit led, Suit trump) const;


	Card min_mil_trl(Suit led, Suit trump, const uint8_t* = nullptr) const;
	Card max_mil_trl(Suit led, Suit trump) const;

	double prob_lt(const Card&, Suit led, Suit trump, int k) const;
	Hand gt_prob(const Hand&, Suit led, Suit trump, int k,
			double prob_floor, double trump_prb_cap = 1, double prob_ceiling = 1) const;
	Card min_gt_prob(const Hand&, Suit led, Suit trump, int k,
			double prob_floor, double trump_prob_cap = 1, double prob_ceiling = 1) const;
	Card max_prob_trl(const Hand &rhs, Suit led, Suit trump, int k,
			double prob_cap = 1, double min_prb = 0, double trump_prb_cap = 1) const;

	bool card_is_valid(const Card&, Suit led) const;

	int diff_between(const Card &c_low, const Card &c_hi, Suit trump = Card::NON_SU) const;


	static const Hand EMPTY;

};



#endif /* HAND_H_ */
