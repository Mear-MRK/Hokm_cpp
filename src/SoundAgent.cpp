/*
 * SoundAgent.cpp
 *
 *  Created on: May 29, 2023
 *      Author: mear
 */

#include "SoundAgent.h"

#include <iostream>
#include <cstring>
#include <string>
#include <cassert>
// #include <iomanip>

#include "CardProb.h"
#include "utils.h"

// n choose k ont in n_ex
static inline double chs_prob(int n, int k, int n_ex)
{
	assert(k <= Card::N_CARDS && n_ex <= Card::N_CARDS && n <= Card::N_CARDS);
	assert(k >= 0 && n_ex >= 0 && n >= 0);

	if (n < k + n_ex)
		return 0;
	double prb = 1;
	for (int i = 0; i < k; i++)
		prb *= static_cast<double>(n - n_ex - i) / (n - i);
	return prb;
}

std::pair<Card, double> SoundAgent::prb_act(const Hand &ps_hi, const Hand &ps_oth, int N_ex, Suit led, Suit trump, bool critical) const
{
	Card c = Card::NONE;
	double max_prb = 0;
	Card max_c = Card::NONE;
	for (Suit s = 0; s < Card::N_SUITS; s++)
	{
		for (int i = 0; i < ps_hi.len[s]; i++)
		{
			Rank r = ps_hi.cards[s][i];
			c.set(s, r);
			int k = ps_oth.gt_c_led(c, led, trump).nbr_cards; // not exactly correct; s may all be in comp
			double prb = chs_prob(ps_oth.nbr_cards, k, N_ex);
			LOG("prb_act " + c.to_string() + " " << prb);
			if (prb > max_prb)
			{
				max_prb = prb;
				max_c = c;
			}
			if (prb <= prob_floor)
				continue;
			if (s != trump || hand->len[trump] == hand->nbr_cards)
			{
				if (prb <= prob_ceiling || critical)
				{
					return std::make_pair(c, prb);
				}
			}
			else if (prb < trump_prob_cap || critical || r < (2*Card::N_RANKS/3) ||  ps_hi.len[trump] > i + 2)
				return std::make_pair(c, prb);
		}
	}
	return std::make_pair((critical) ? max_c : Card::NONE, max_prb);
}

Suit SoundAgent::call_trump(const CardStack &first_5cards)
{
	Hand hand = first_5cards.to_Hand();

	LOG(name << ", trump call, first5:\n"
			 << hand.to_su_string());

	float scr[Card::N_SUITS];

	float beta = 1.0f / 6;

	for (Suit s = 0; s < Card::N_SUITS; s++)
	{
		float sum_r = 0;
		for (int i = 0; i < hand.len[s]; i++)
			sum_r += hand.cards[s][i];
		scr[s] = hand.len[s] + beta * sum_r;
	}

	double max_scr = -1;
	Suit trump = Card::NON_SU;
	for (Suit t = 0; t < Card::N_SUITS; t++)
	{
		LOG(name << ", trump call, suit " << Card::SU_STR[t] << ", score: " << scr[t]);
		if (scr[t] > max_scr)
		{
			max_scr = scr[t];
			trump = t;
		}
		else if (scr[t] == max_scr && hand.len[t] < hand.len[trump])
			trump = t;
	}
	LOG(name << ", alg. trump: " << Card::SU_STR[trump]);
	//	if (trump == Card::NON_SU)
	//		return first_5cards.at(
	//				std::uniform_int_distribution<int>(0, 4)(mt_rnd_gen)).su;
	return trump;
}

void SoundAgent::init_round(Hand &hand)
{

	this->hand = &hand;

	Habc = this->hand->cmpl();
	LOG(name << " Habc: " << Habc.to_string());
	Ha = Hb = Hc = Hab = Hbc = Hca = Hand::EMPTY;

	last_ord = -1;
	last_led = Card::NON_SU;
}

void SoundAgent::update_oth_hands(int pl_id, const Card &pl_c, Suit led)
{
#ifdef DEBUG
	if (pl_id == player_id)
		throw std::invalid_argument(
			"SoundAgent::update_oth_hands : pl_id shouldn't be player_id");
	if (pl_c == Card::NONE)
		throw std::invalid_argument(
			"SoundAgent::update_oth_hands : pl_c shouldn't be none card");
#endif

	if ((player_id + 2) % Hokm::N_PLAYERS == pl_id) // c
	{
		if (!(Habc.remove(pl_c) || Hca.remove(pl_c) || Hbc.remove(pl_c) || Hc.remove(pl_c)))
		{
			std::cerr << "The card is nowhere! c: " << pl_c.to_string() << " H: "
					  << (Hc.uni(Hca).uni(Hbc).uni(Habc)).to_string() << std::endl;
		}
		if (led != Card::NON_SU && pl_c.su != led)
		{
			Hab.add(Habc.pop(led));
			Ha.add(Hca.pop(led));
			Hb.add(Hbc.pop(led));
		}
	}
	else if ((player_id + 1) % Hokm::N_PLAYERS == pl_id) // a
	{
		if (!(Habc.remove(pl_c) || Hca.remove(pl_c) || Hab.remove(pl_c) || Ha.remove(pl_c)))
		{
			std::cerr << "The card is nowhere! a: " << pl_c.to_string() << " H: " << (Ha.uni(Hca).uni(Hab).uni(Habc)).to_string() << std::endl;
		}
		if (led != Card::NON_SU && pl_c.su != led)
		{
			Hbc.add(Habc.pop(led));
			Hc.add(Hca.pop(led));
			Hb.add(Hab.pop(led));
		}
	}
	else // b
	{
		if (!(Habc.remove(pl_c) || Hab.remove(pl_c) || Hbc.remove(pl_c) || Hb.remove(pl_c)))
		{
			std::cerr << "The card is nowhere! b: " << pl_c.to_string() << " H: " << (Hb.uni(Hbc).uni(Hab).uni(Habc)).to_string() << std::endl;
		}
		if (led != Card::NON_SU && pl_c.su != led)
		{
			Hca.add(Habc.pop(led));
			Hc.add(Hbc.pop(led));
			Ha.add(Hab.pop(led));
		}
	}
}

Card SoundAgent::act(const State &state, const History &hist)
{
	int op_team = (team_id + 1) % Hokm::N_TEAMS;
	LOG(
		"--- " << name << " pl_id " << player_id << " team " << team_id
			   << " ord " << (int)state.ord
			   << " last_ord " << last_ord << " last_led "
			   << Card::SU_STR[last_led] << " led " << Card::SU_STR[state.led]
			   << " trump " << Card::SU_STR[state.trump]
			   << "\nhand: " << hand->to_string());

	int b_id = (player_id + 3) % 4;
	int c_id = (player_id + 2) % 4;
	int a_id = (player_id + 1) % 4;

	bool critical = state.score[op_team] >= (Hokm::RND_WIN_SCORE - 2) ||
					state.score[op_team] > state.score[team_id] + Hokm::RND_WIN_SCORE / 2;

	Card c;
	switch (last_ord)
	{
	case -1:
		break;
	case 0:
		c = hist.played_cards[b_id].at(-1);
		LOG("op_b " << b_id << " last table " << c.to_string());
		update_oth_hands(b_id, c, last_led);
	case 1:
		c = hist.played_cards[c_id].at(-1);
		LOG("comp " << c_id << " last table " << c.to_string());
		update_oth_hands(c_id, c, last_led);
	case 2:
		c = hist.played_cards[a_id].at(-1);
		LOG("op_a " << a_id << " last table " << c.to_string());
		update_oth_hands(a_id, c, last_led);
	}

	switch (state.ord)
	{
	case 3:
		c = state.table[a_id];
		LOG("op_a " << a_id << " table " << c.to_string());
		update_oth_hands(a_id, c, state.led);
	case 2:
		c = state.table[c_id];
		LOG("comp " << c_id << " table " << c.to_string());
		update_oth_hands(c_id, c, state.led);
	case 1:
		c = state.table[b_id];
		LOG("op_b " << b_id << " table " << c.to_string());
		update_oth_hands(b_id, c, state.led);
	}
	last_ord = state.ord;
	last_led = state.led;

	const Card &a_card = state.table[a_id];
	const Card &b_card = state.table[b_id];
	const Card &c_card = state.table[c_id];

	Card out = Card::NONE;

	switch (state.ord)
	{

	case 0:
	{
		LOG("case 0:");
		assert(a_card == Card::NONE && b_card == Card::NONE && c_card == Card::NONE);

		Hand H_ab = Hab.uni(Ha).uni(Hb);
		Hand ps_hi = hand->lead_gt(H_ab, Card::NON_SU, state.trump);
		LOG("pssbl_hi_hand: " << ps_hi.to_string());
		if (ps_hi.nbr_cards == 0)
		{
			out = hand->min_mil_trl(Card::NON_SU, state.trump);
			break;
		}
		Hand ps_abc = Habc.uni(Hca).uni(Hbc);
		// Hand ps_hi_c = Hc.lead_gt(H_ab, Card::NON_SU, state.trump);
		int Ncards = Hokm::N_DELT - state.trick_id;
		int Nu_c = Ncards - Hc.nbr_cards;
		int Nu_ab = 2 * Ncards - Ha.nbr_cards - Hb.nbr_cards - Hab.nbr_cards;
		assert(Nu_c + Nu_ab == ps_abc.nbr_cards);

		auto out_prb = prb_act(ps_hi, ps_abc, Nu_ab, Card::NON_SU, state.trump, critical);
		out = out_prb.first;
		LOG("out: " << out.to_string() << ", prb: " << out_prb.second);
		if (out == Card::NONE)
			out = hand->min_mil_trl(Card::NON_SU, state.trump);
	}
	break;

	case 1:
	{
		LOG("case 1:");
		assert(a_card == Card::NONE && b_card != Card::NONE && c_card == Card::NONE);

		Hand H_a = Ha;
		H_a.add(b_card);
		Hand ps_hi = hand->lead_gt(H_a, state.led, state.trump);
		LOG("pssbl_hi_hand: " << ps_hi.to_string());
		if (ps_hi.nbr_cards == 0)
		{
			out = hand->min_mil_trl(state.led, state.trump);
			break;
		}
		Hand ps_abc = Habc.uni(Hca).uni(Hab);
		// Hand ps_hi_c = Hc.lead_gt(H, state.led, state.trump);
		int Ncards = Hokm::N_DELT - state.trick_id;
		int Nu_c = Ncards - Hc.nbr_cards;
		int Nu_a = Ncards - Ha.nbr_cards;
		int Nu_b = Ncards - 1 - Hb.nbr_cards;
		// double rc = (double)Nu_c / (Nu_a + Nu_b + Nu_c), rb = (double)Nu_b / (Nu_a + Nu_b + Nu_c);
		// double rc_ca = (double)Nu_c / (Nu_a + Nu_c), rb_ab = (double)Nu_b / (Nu_a + Nu_b);
		// int pNu_c = round(rc * Habc.nbr_cards + rc_ca * Hca.nbr_cards);
		// int pNu_b = round(rb * Habc.nbr_cards + rb_ab * Hab.nbr_cards);
		auto out_prb = prb_act(ps_hi, ps_abc, Nu_a, state.led, state.trump, critical);
		out = out_prb.first;
		LOG("out: " << out.to_string() << ", prb: " << out_prb.second);
		if (out == Card::NONE)
			out = hand->min_mil_trl(state.led, state.trump);
	}
	break;

	case 2:
	{
		LOG("case 2:");
		assert(a_card == Card::NONE && b_card != Card::NONE && c_card != Card::NONE);

		Hand H_a = Ha;
		H_a.add(b_card);
		Hand ps_hi = hand->lead_gt(H_a, state.led, state.trump);
		LOG("pssbl_hi_hand: " << ps_hi.to_string());
		if (ps_hi.nbr_cards == 0)
		{
			out = hand->min_mil_trl(state.led, state.trump);
			break;
		}
		Hand ps_abc = Habc.uni(Hca).uni(Hab);
		int Ncards = Hokm::N_DELT - state.trick_id;
		int Nu_c = Ncards - 1 - Hc.nbr_cards;
		int Nu_a = Ncards - Ha.nbr_cards;
		int Nu_b = Ncards - 1 - Hb.nbr_cards;

		auto out_prb = prb_act(ps_hi, ps_abc, Nu_a, state.led, state.trump, critical);
		out = out_prb.first;
		double prb = out_prb.second;
		LOG("prb_act out: " << out.to_string() << ", (max)prb: " << prb);

		if (out == Card::NONE)
		{
			out = hand->min_mil_trl(state.led, state.trump);
			break;
		}
		int k = ps_abc.gt_c_led(c_card, state.led, state.trump).nbr_cards;
		double c_prb = chs_prob(ps_abc.nbr_cards, k, Nu_a);
		LOG("c_card: " << c_card.to_string() << ", prb: " << c_prb);
		H_a.add(ps_abc);
		int dfn = H_a.diff_between(out, c_card, state.trump);
		LOG("diff of " << c_card.to_string() << "-" << out.to_string() << ": " << dfn);
		if (critical && prb > c_prb)
		{
			LOG("Critical");
		}
		else if (c_prb > prob_floor || dfn >= 0)
		{
			LOG("Companion's card is good enough.");
			out = hand->min_mil_trl(state.led, state.trump);
		}
	}
	break;

	case 3:
	{
		LOG("case 3:");
		Card bst_op =
			(Card::cmp(b_card, a_card, state.led, state.trump) > 0) ? b_card : a_card;
		if (Card::cmp(c_card, bst_op, state.led, state.trump) > 0)
		{
			LOG("Companion's card is good enough.");
			out = hand->min_mil_trl(state.led, state.trump);
			break;
		}
		Hand hi_hand = hand->gt_c_led(bst_op, state.led, state.trump);
		LOG("hi_hand: " << hi_hand.to_string());
		if (hi_hand.nbr_cards)
		{
			out = hi_hand.min_mil_trl(state.led, state.trump);
		}
		else
			out = hand->min_mil_trl(state.led, state.trump);
	}
	break;
	default:
		throw std::logic_error("Shouldn't be here!");
	}
	LOG("Alg. out card: " << out.to_string());
	hand->remove(out);
	LOG(
		"--- " << name << " --- out card: " << out.to_string() << " ---");
	return out;
}

SoundAgent::SoundAgent(int player_id, double min_prob, double trump_prb_cap, double max_prob)
	: Agent(player_id, "AI_" + std::to_string(player_id)),
	  mt_rnd_gen(std::mt19937(std::random_device()())),
	  prob_floor(min_prob),
	  prob_ceiling(max_prob),
	  trump_prob_cap(trump_prb_cap)
{
}

void SoundAgent::set_probs(double prob_floor, double trump_prob_cap,
						   double prob_ceiling)
{
	//	if (prob_floor < 1 && prob_ceiling > prob_floor && prob_ceiling > 0
	//			&& trump_prob_cap > 0 && trump_prob_cap > prob_floor){
	this->prob_floor = prob_floor;
	this->prob_ceiling = prob_ceiling;
	this->trump_prob_cap = trump_prob_cap;
	//	}
}

void SoundAgent::reset()
{

	Habc.reset();
	Hab.reset();
	Hbc.reset();
	Hca.reset();
	Ha.reset();
	Hb.reset();
	Hc.reset();

	last_ord = -1;
	last_led = Card::NON_SU;
}
