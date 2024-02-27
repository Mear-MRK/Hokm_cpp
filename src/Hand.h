#pragma once

#include <string>
#include <cstdint>

#include "Card.h"

class Hand {

public:

	Rank cards[Card::N_SUITS][Card::N_RANKS];
	int len[Card::N_SUITS];
	std::uint64_t bin64;
	int nbr_cards;

	Hand();
	Hand(const Card *cards, int nbr_cards);
	Hand(std::uint64_t bin64);

	Hand& add(const Card &c);
	Hand& add(const Card *cards, int nbr_cards);
	Hand& add(const Hand &);

	Card pop(Suit su, int ind);
	Hand pop(Suit su);

	Hand& rm(const Card &c);
	bool remove(const Card &c);
	Hand& discard(Suit);

	bool is_in(const Card &c) const;

	Hand sub_hand(Suit su) const;
	Hand sub_hand_sup_rank(Suit su, Rank rnk) const;

	std::string to_string() const;
	std::string to_su_string() const;

	Hand& clear();

	Hand cmpl() const;
	Hand uni(const Hand&) const;
	Hand inter(const Hand&) const;

	Hand lead_gt(const Card&, Suit led, Suit trump) const;
	Hand gt_c_led(const Card &card, Suit led, Suit trump) const;

	Hand lead_gt(const Hand&, Suit led, Suit trump) const;
	Hand gt_h_lead(const Hand& h, Suit led, Suit trump) const;
	Hand gt(const Card&, Suit led, Suit trump) const;


	Card min_mil_trl(Suit led, Suit trump, const int* = nullptr) const;
	Card max_mil_trl(Suit led, Suit trump) const;

	bool card_is_valid_move(const Card&, Suit led) const;

	int diff_between(const Card &c_low, const Card &c_hi, Suit trump = Card::NON_SU) const;

	static const Hand EMPTY;

};
