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

#include "utils.h"

/*
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
			if (s != trump || hand.len[trump] == hand.nbr_cards)
			{
				if (prb <= prob_ceiling || critical)
				{
					return std::make_pair(c, prb);
				}
			}
			else if (prb < trump_prob_cap || critical || r < (2 * Card::N_RANKS / 3) || ps_hi.len[trump] > i + 2)
				return std::make_pair(c, prb);
		}
	}
	return std::make_pair((critical) ? max_c : Card::NONE, max_prb);
}
*/
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

void SoundAgent::init_round(const Hand &hand)
{

	this->hand = hand;

	Habc = this->hand.cmpl();
	Ha = Hb = Hc = Hab = Hbc = Hca = Hand::EMPTY;
	Nu_a = Nu_b = Nu_c = Hokm::N_DELT;
	last_ord = -1;
	last_led = Card::NON_SU;
}

void SoundAgent::update_oth_hands(int pl_id, const Card &pl_card, Suit led)
{
#ifdef DEBUG
	if (pl_id == player_id)
		throw std::invalid_argument(
			"SoundAgent::update_oth_hands : pl_id shouldn't be player_id");
	if (pl_card == Card::NONE)
		throw std::invalid_argument(
			"SoundAgent::update_oth_hands : pl_c shouldn't be none card");
#endif

	if ((player_id + 2) % Hokm::N_PLAYERS == pl_id) // c
	{

		if (!(Habc.remove(pl_card) || Hca.remove(pl_card) || Hbc.remove(pl_card)))
		{
			if (!Hc.remove(pl_card))
				std::cerr << "The card is nowhere! c: " << pl_card.to_string() << " H: "
						  << (Hc.uni(Hca).uni(Hbc).uni(Habc)).to_string() << std::endl;
		}
		if (led != Card::NON_SU && pl_card.su != led)
		{
			Hab.add(Habc.pop(led));
			Hand h = Hca.pop(led);
			Ha.add(h);
			h = Hbc.pop(led);
			Hb.add(h);
		}
	}
	else if ((player_id + 1) % Hokm::N_PLAYERS == pl_id) // a
	{
		if (!(Habc.remove(pl_card) || Hca.remove(pl_card) || Hab.remove(pl_card)))
		{
			if (!Ha.remove(pl_card))
				std::cerr << "The card is nowhere! a: " << pl_card.to_string() << " H: " << (Ha.uni(Hca).uni(Hab).uni(Habc)).to_string() << std::endl;
		}
		if (led != Card::NON_SU && pl_card.su != led)
		{
			Hbc.add(Habc.pop(led));
			Hand h = Hca.pop(led);
			Hc.add(h);
			h = Hab.pop(led);
			Hb.add(h);
		}
	}
	else // b
	{
		if (!(Habc.remove(pl_card) || Hab.remove(pl_card) || Hbc.remove(pl_card)))
		{
			if (!Hb.remove(pl_card))
				std::cerr << "The card is nowhere! b: " << pl_card.to_string() << " H: " << (Hb.uni(Hbc).uni(Hab).uni(Habc)).to_string() << std::endl;
		}

		if (led != Card::NON_SU && pl_card.su != led)
		{
			Hca.add(Habc.pop(led));
			Hand h = Hbc.pop(led);
			Hc.add(h);
			h = Hab.pop(led);
			Ha.add(h);
		}
	}
}

void SoundAgent::updateHandsNcards()
{
	int &Nabc = Habc.nbr_cards;
	int &Nab = Hab.nbr_cards;
	int &Nca = Hca.nbr_cards;
	int &Nbc = Hbc.nbr_cards;
	int &Na = Ha.nbr_cards;
	int &Nb = Hb.nbr_cards;
	int &Nc = Hc.nbr_cards;

	bool changed = false;
	do
	{
		changed = false;
		if (Ncards_a == Na && (Nabc + Nab + Nca))
		{
			Hbc.add(Habc);
			Habc.clear();
			Hb.add(Hab);
			Hab.clear();
			Hc.add(Hca);
			Hca.clear();
			changed = true;
		}
		if (Ncards_b == Nb && (Nabc + Nab + Nbc))
		{
			Hca.add(Habc);
			Habc.clear();
			Ha.add(Hab);
			Hab.clear();
			Hc.add(Hbc);
			Hbc.clear();
			changed = true;
		}
		if (Ncards_c == Nc && (Nabc + Nbc + Nca))
		{
			Hab.add(Habc);
			Habc.clear();
			Ha.add(Hca);
			Hca.clear();
			Hb.add(Hbc);
			Hbc.clear();
			changed = true;
		}
		if (Ncards_a + Ncards_b == Na + Nb + Nab && (Nabc + Nca + Nbc))
		{
			Hc.add(Habc);
			Habc.clear();
			Hc.add(Hca);
			Hca.clear();
			Hc.add(Hbc);
			Hbc.clear();
			changed = true;
		}
		if (Ncards_a + Ncards_c == Na + Nc + Nca && (Nabc + Nab + Nbc))
		{
			Hb.add(Habc);
			Habc.clear();
			Hb.add(Hab);
			Hab.clear();
			Hb.add(Hbc);
			Hbc.clear();
			changed = true;
		}
		if (Ncards_b + Ncards_c == Nb + Nc + Nbc && (Nabc + Nab + Nca))
		{
			Ha.add(Habc);
			Habc.clear();
			Ha.add(Hab);
			Hab.clear();
			Ha.add(Hca);
			Hca.clear();
			changed = true;
		}
	} while (changed);
}


// {Nabc -> 0, Nab -> 0, Nca -> 0, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0000(double p[9], double, double, double, double, double ua, double ub, double uc) {
	assert(ua==0 && ub==0 && uc==0);
p[0] = 0.3333333333333333;
p[1] = 0.5;
p[2] = 0.5;
p[3] = 0.3333333333333333;
p[4] = 0.5;
p[5] = 0.5;
p[6] = 0.3333333333333333;
p[7] = 0.5;
p[8] = 0.5;
}


// {Nabc -> 0, Nab -> 0, Nca -> 0, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0001(double p[9], double, double, double, double bc, double ua, double ub, double uc) {
	assert(ua==0);
	assert(ub+uc==bc);
p[0] = 0.3333333333333333;
p[1] = 0.5;
p[2] = 0.5;
p[3] = 0.3333333333333333;
p[4] = 0.5;
p[5] = ub/bc;
p[6] = 0.3333333333333333;
p[7] = 0.5;
p[8] = uc/bc;
}


// {Nabc -> 0, Nab -> 0, Nca -> ca, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0010(double p[9], double, double, double ca, double, double ua, double ub, double uc) {
	assert(ub==0);
	assert(uc+ua==ca);
p[0] = 0.3333333333333333;
p[1] = 0.5;
p[2] = ua/ca;
p[3] = 0.3333333333333333;
p[4] = 0.5;
p[5] = 0.5;
p[6] = 0.3333333333333333;
p[7] = uc/ca;
p[8] = 0.5;
}


// {Nabc -> 0, Nab -> 0, Nca -> ca, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0011(double p[9], double, double, double ca, double bc, double ua, double ub, double uc) {
	assert(ca+bc==ua+ub+uc);
p[0] = 0.3333333333333333;
p[1] = 0.5;
p[2] = ua / ca;
p[3] = 0.3333333333333333;
p[4] = 0.5;
p[5] = ub / bc;
p[6] = 0.3333333333333333;
p[7] = 1 - p[2];
p[8] = 1 - p[5];
}


// {Nabc -> 0, Nab -> ab, Nca -> 0, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0100(double p[9], double, double ab, double, double, double ua, double ub, double uc) {
	assert(uc==0);
	assert(ua+ub==ab);
p[0] = 0.3333333333333333;
p[1] = ua / ab;
p[2] = 0.5;
p[3] = 0.3333333333333333;
p[4] = ub / ab;
p[5] = 0.5;
p[6] = 0.3333333333333333;
p[7] = 0.5;
p[8] = 0.5;
}


// {Nabc -> 0, Nab -> ab, Nca -> 0, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0101(double p[9], double, double ab, double, double bc, double ua, double ub, double uc) {
	assert(ua + ub + uc == ab + bc);
p[0] = 0.3333333333333333;
p[1] = ua / ab;
p[2] = 0.5;
p[3] = 0.3333333333333333;
p[4] = 1 - p[1];
p[8] = uc / bc;
p[5] = 1 - p[8];
p[6] = 0.3333333333333333;
p[7] = 0.5;
}


// {Nabc -> 0, Nab -> ab, Nca -> ca, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0110(double p[9], double, double ab, double ca, double, double ua, double ub, double uc) {
	assert(ua + ub + uc == ab + ca);
p[0] = 0.3333333333333333;
p[4] = ub / ab;
p[1] = 1 - p[4];
p[7] = uc / ca;
p[2] = 1 - p[7];
p[3] = 0.3333333333333333;
p[5] = 0.5;
p[6] = 0.3333333333333333;
p[8] = 0.5;
}


// {Nabc -> 0, Nab -> ab, Nca -> ca, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb0111(double p[9], double, double ab, double ca, double bc, double ua, double ub, double uc) {
	assert(ua + ub + uc == ab + ca + bc);
	double uap = ua * (1 - ab/(ua+ub) - ca/(uc+ua));
	double ubp = ub * (1 - ab/(ua+ub) - bc/(ub+uc));
	double ucp = uc * (1 - ca/(uc+ua) - bc/(ub+uc));
	double eta = 0;
	double x = (uap - ubp) / 3 + eta;
	double y = (ubp - ucp) / 3 + eta;
	double z = (ucp - uap) / 3 + eta;
p[0] = 0.3333333333333333;
p[1] = ua / (ua + ub) + x / ab;
p[2] = ua / (uc + ua) - z / ca;
p[3] = 0.3333333333333333;
p[4] = ub / (ua + ub) - x / ab;
p[5] = ub / (ub + uc) + y / bc;
p[6] = 0.3333333333333333;
p[7] = uc / (uc + ua) + z / ca;
p[8] = uc / (ub + uc) - y / bc;
}


// {Nabc -> w, Nab -> 0, Nca -> 0, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1000(double p[9], double w, double, double, double, double ua, double ub, double uc) {
	assert(ua + ub + uc == w);
p[0] = ua/w;
p[1] = 0.5;
p[2] = 0.5;
p[3] = ub/w;
p[4] = 0.5;
p[5] = 0.5;
p[6] = uc/w;
p[7] = 0.5;
p[8] = 0.5;
}


// {Nabc -> w, Nab -> 0, Nca -> 0, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1001(double p[9], double w, double, double, double bc, double ua, double ub, double uc) {
	assert(w + bc == ua + ub + uc);
	assert(w >= ua);
p[0] = ua / w;
p[1] = 0.5;
p[2] = 0.5;
p[3] = (1 - p[0]) * ub / (ub + uc);
p[4] = 0.5;
p[5] = ub / (ub + uc);
p[6] = (1 - p[0]) * uc / (ub + uc);
p[7] = 0.5;
p[8] = uc / (ub + uc);
}


// {Nabc -> w, Nab -> 0, Nca -> ca, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1010(double p[9], double w, double, double ca, double, double ua, double ub, double uc) {
	assert(w + ca == ua + ub + uc);
	assert(w >= ub);
p[3] = ub / w;
p[0] = (1 - p[3]) * ua / (ua + uc);
p[1] = 0.5;
p[2] = ua / (ua + uc);
p[4] = 0.5;
p[5] = 0.5;
p[6] = (1 - p[3]) * uc / (ua + uc);
p[7] = uc / (ua + uc);
p[8] = 0.5;
}


// {Nabc -> w, Nab -> 0, Nca -> ca, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1011(double p[9], double w, double, double ca, double bc, double ua, double ub, double uc) {
p[0] = (std::pow(bc,3)*(4 + std::pow(ca,2))*w + std::pow(bc,4)*(std::pow(ca,2) - 2*ca*w + 4*ua*w) + std::pow(bc,2)*(std::pow(ca,2)*(3 + std::pow(ca,2)) - 2*(ca + std::pow(ca,2)*(ca - 2*ua + ub) + 2*(-2*ua + ub + uc))*w + (4 + 3*std::pow(ca,2))*std::pow(w,2) - 4*(ca - 2*ua)*std::pow(w,3)) + 2*w*(std::pow(ca,2)*(ua - 2*ub - ca*(1 + ca*ub) + uc) + std::pow(ca,2)*(2 + std::pow(ca,2))*w - (ca + std::pow(ca,2)*(ca - 2*ua + ub) + 2*(-2*ua + ub + uc))*std::pow(w,2) + (2 + std::pow(ca,2))*std::pow(w,3) - (ca - 2*ua)*std::pow(w,4)) + bc*w*(std::pow(ca,4) + 4*std::pow(w,2) + std::pow(ca,2)*(1 + std::pow(w,2))))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[1] = 0.5;
p[2] = (3*std::pow(bc,2)*ca*(bc + ca + 2*ua + (std::pow(bc,2) + std::pow(ca,2))*ua - ub - uc) - std::pow(bc,2)*ca*(std::pow(bc,2) + std::pow(ca,2))*w + (2*std::pow(bc,4) - std::pow(bc,3)*ca - bc*ca*(-3 + std::pow(ca,2)) + std::pow(bc,2)*(6 + ca*(2*ca + 7*ua + 2*ub)) + 2*ca*(ca*(3 + ca*(2*ua + ub)) + 3*(ua - uc)))*std::pow(w,2) - ca*(3*std::pow(bc,2) + 2*std::pow(ca,2))*std::pow(w,3) + (6 + 4*std::pow(bc,2) - bc*ca + 2*ca*(ca + 2*ua + ub))*std::pow(w,4) - 2*ca*std::pow(w,5) + 2*std::pow(w,6))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[3] = (-2*std::pow(bc,3)*w*(1 + std::pow(ca,2) + std::pow(w,2)) - 2*bc*w*(std::pow(ca,2) + std::pow(w,2))*(1 + std::pow(ca,2) + std::pow(w,2)) + std::pow(bc,4)*(std::pow(ca,2) + ca*w + 2*w*(-ua + w)) + 4*w*(std::pow(ca,2) + std::pow(w,2))*(ca - ua + std::pow(ca,2)*ub - uc + w + ub*(2 + std::pow(w,2))) + std::pow(bc,2)*(std::pow(ca,4) + std::pow(ca,3)*w + ca*(w + std::pow(w,3)) + std::pow(ca,2)*(3 + w*(-2*ua + 4*ub + 3*w)) + 2*w*(ub + uc + 2*w + 2*ub*std::pow(w,2) + std::pow(w,3) - ua*(2 + std::pow(w,2)))))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[4] = 0.5;
p[5] = (3*bc*std::pow(ca,2)*(bc + ca - ua + 2*ub + (std::pow(bc,2) + std::pow(ca,2))*ub - uc) - bc*std::pow(ca,2)*(std::pow(bc,2) + std::pow(ca,2))*w + (2*std::pow(bc,2)*(3 + std::pow(ca,2)) + 2*std::pow(ca,2)*(3 + std::pow(ca,2)) + std::pow(bc,3)*(-ca + 2*ua + 4*ub) + bc*ca*(3 + ca*(-ca + 2*ua + 7*ub)) + 6*bc*(ub - uc))*std::pow(w,2) - bc*(2*std::pow(bc,2) + 3*std::pow(ca,2))*std::pow(w,3) + (6 + 2*std::pow(bc,2) + 4*std::pow(ca,2) + bc*(-ca + 2*ua + 4*ub))*std::pow(w,4) - 2*bc*std::pow(w,5) + 2*std::pow(w,6))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[6] = (std::pow(bc,2)*std::pow(ca,2)*(3 + std::pow(bc,2) + std::pow(ca,2)) + (-2*std::pow(bc,3)*(1 + std::pow(ca,2)) + bc*(std::pow(ca,2) + std::pow(ca,4)) + std::pow(bc,4)*(ca - 2*ua) + 2*std::pow(ca,2)*(ua - 2*ub - ca*(1 + ca*ub) + uc) + std::pow(bc,2)*(ca + 2*(-2*ua + ub + uc) + std::pow(ca,2)*(-2*ca + ua + ub + 3*uc)))*w + 2*(std::pow(bc,4) + 2*std::pow(ca,2) + std::pow(ca,4) + std::pow(bc,2)*(2 + std::pow(ca,2)))*std::pow(w,2) - (2*std::pow(bc,3) + bc*(2 + std::pow(ca,2)) + 2*(ca + std::pow(ca,2)*(ca + ub - 2*uc) + 2*(ua + ub - 2*uc)) + std::pow(bc,2)*(ca + 2*ua - 4*uc))*std::pow(w,3) + 2*(2 + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,4) - 2*(bc + ca - 2*uc)*std::pow(w,5))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[7] = (-3*std::pow(bc,3)*ca*(1 + std::pow(ca,2) + std::pow(w,2)) - 3*bc*ca*std::pow(w,2)*(1 + std::pow(ca,2) + std::pow(w,2)) + std::pow(bc,4)*(3*ca*(ca - ua) + ca*w + 2*std::pow(w,2)) + 2*std::pow(w,2)*(ca*(-3*ua + 3*uc + ca*(3 + ca*(ub + 2*uc))) - std::pow(ca,3)*w + (3 + ca*(ca + ub + 2*uc))*std::pow(w,2) - ca*std::pow(w,3) + std::pow(w,4)) + std::pow(bc,2)*(3*ca*(-2*ua + ub + uc + ca*(2 + ca*(ub + uc))) - 2*std::pow(ca,3)*w + (6 + ca*(5*ca - 3*ua + 2*ub + 4*uc))*std::pow(w,2) - ca*std::pow(w,3) + 4*std::pow(w,4)))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[8] = (std::pow(bc,2)*(std::pow(ca,2) + std::pow(w,2))*(6 + 3*std::pow(ca,2) + 2*std::pow(w,2)) + std::pow(bc,3)*(3*std::pow(ca,2)*(-ca + ua + uc) - 2*std::pow(ca,2)*w + (-3*ca + 2*ua + 4*uc)*std::pow(w,2) - 2*std::pow(w,3)) + (std::pow(ca,2) + std::pow(w,2))*(3*(2 + std::pow(ca,2))*ub*(ca - ua - ub - uc) + 3*(2 + std::pow(ca,2))*ub*w + 2*(3 + std::pow(ca,2))*std::pow(w,2) + 2*std::pow(w,4)) + bc*(3*std::pow(ca,2)*(-ca + ua + uc) + std::pow(ca,4)*w + (6*uc + ca*(-3 + ca*(-3*ca + 2*ua + 4*uc)))*std::pow(w,2) - std::pow(ca,2)*std::pow(w,3) + (-3*ca + 2*ua + 4*uc)*std::pow(w,4) - 2*std::pow(w,5)))/((3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 4*(std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
}


// {Nabc -> w, Nab -> ab, Nca -> 0, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1100(double p[9], double w, double ab, double, double, double ua, double ub, double uc) {
	assert(w + ab == ua + ub + uc);
	assert(w >= uc);
p[6] = uc / w;
p[0] = (1 - p[6]) * ua / (ua + ub);
p[1] = ua / (ua + ub);
p[2] = 0.5;
p[3] = (1 - p[6]) * ub / (ua + ub);
p[4] = ub / (ua + ub);
p[5] = 0.5;
p[7] = 0.5;
p[8] = 0.5;
}


// {Nabc -> w, Nab -> ab, Nca -> 0, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1101(double p[9], double w, double ab, double, double bc, double ua, double ub, double uc) {
p[0] = (-2*std::pow(ab,3)*w*(1 + std::pow(bc,2) + std::pow(w,2)) - 2*ab*w*(std::pow(bc,2) + std::pow(w,2))*(1 + std::pow(bc,2) + std::pow(w,2)) + std::pow(ab,4)*(std::pow(bc,2) + bc*w + 2*w*(-uc + w)) + 4*w*(std::pow(bc,2) + std::pow(w,2))*(bc + std::pow(bc,2)*ua - ub - uc + w + ua*(2 + std::pow(w,2))) + std::pow(ab,2)*(std::pow(bc,4) + std::pow(bc,3)*w + bc*(w + std::pow(w,3)) + std::pow(bc,2)*(3 + w*(4*ua - 2*uc + 3*w)) + 2*w*(ua + ub + 2*ua*std::pow(w,2) - (uc - w)*(2 + std::pow(w,2)))))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[1] = (3*ab*std::pow(bc,2)*(ab + bc + 2*ua + (std::pow(ab,2) + std::pow(bc,2))*ua - ub - uc) - ab*std::pow(bc,2)*(std::pow(ab,2) + std::pow(bc,2))*w + (2*std::pow(ab,2)*(3 + std::pow(bc,2)) + 2*std::pow(bc,2)*(3 + std::pow(bc,2)) + 6*ab*(ua - ub) + std::pow(ab,3)*(-bc + 4*ua + 2*uc) + ab*bc*(3 + bc*(-bc + 7*ua + 2*uc)))*std::pow(w,2) - ab*(2*std::pow(ab,2) + 3*std::pow(bc,2))*std::pow(w,3) + (6 + 2*std::pow(ab,2) + 4*std::pow(bc,2) + ab*(-bc + 4*ua + 2*uc))*std::pow(w,4) - 2*ab*std::pow(w,5) + 2*std::pow(w,6))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[2] = 0.5;
p[3] = (std::pow(ab,2)*std::pow(bc,2)*(3 + std::pow(ab,2) + std::pow(bc,2)) + (-2*std::pow(ab,3)*(1 + std::pow(bc,2)) + ab*(std::pow(bc,2) + std::pow(bc,4)) + std::pow(ab,4)*(bc - 2*uc) + 2*std::pow(bc,2)*(-2*ua - bc*(1 + bc*ua) + ub + uc) + std::pow(ab,2)*(bc + 2*(ua + ub - 2*uc) + std::pow(bc,2)*(-2*bc + ua + 3*ub + uc)))*w + 2*(std::pow(ab,4) + 2*std::pow(bc,2) + std::pow(bc,4) + std::pow(ab,2)*(2 + std::pow(bc,2)))*std::pow(w,2) - (2*std::pow(ab,3) + ab*(2 + std::pow(bc,2)) + std::pow(ab,2)*(bc - 4*ub + 2*uc) + 2*(bc + std::pow(bc,2)*(bc + ua - 2*ub) + 2*(ua - 2*ub + uc)))*std::pow(w,3) + 2*(2 + std::pow(ab,2) + std::pow(bc,2))*std::pow(w,4) - 2*(ab + bc - 2*ub)*std::pow(w,5))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[4] = (std::pow(ab,2)*(std::pow(bc,2) + std::pow(w,2))*(6 + 3*std::pow(bc,2) + 2*std::pow(w,2)) + std::pow(ab,3)*(3*std::pow(bc,2)*(-bc + ub + uc) - 2*std::pow(bc,2)*w + (-3*bc + 4*ub + 2*uc)*std::pow(w,2) - 2*std::pow(w,3)) + (std::pow(bc,2) + std::pow(w,2))*(3*(2 + std::pow(bc,2))*ua*(bc - ua - ub - uc) + 3*(2 + std::pow(bc,2))*ua*w + 2*(3 + std::pow(bc,2))*std::pow(w,2) + 2*std::pow(w,4)) + ab*(3*std::pow(bc,2)*(-bc + ub + uc) + std::pow(bc,4)*w + (6*ub + bc*(-3 + bc*(-3*bc + 4*ub + 2*uc)))*std::pow(w,2) - std::pow(bc,2)*std::pow(w,3) + (-3*bc + 4*ub + 2*uc)*std::pow(w,4) - 2*std::pow(w,5)))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[5] = (-3*std::pow(ab,3)*bc*(1 + std::pow(bc,2) + std::pow(w,2)) - 3*ab*bc*std::pow(w,2)*(1 + std::pow(bc,2) + std::pow(w,2)) + std::pow(ab,4)*(3*bc*(bc - uc) + bc*w + 2*std::pow(w,2)) + 2*std::pow(w,2)*(bc*(bc*(3 + bc*(ua + 2*ub)) + 3*(ub - uc)) - std::pow(bc,3)*w + (3 + bc*(bc + ua + 2*ub))*std::pow(w,2) - bc*std::pow(w,3) + std::pow(w,4)) + std::pow(ab,2)*(3*bc*(ua + ub + bc*(2 + bc*(ua + ub)) - 2*uc) - 2*std::pow(bc,3)*w + (6 + bc*(5*bc + 2*ua + 4*ub - 3*uc))*std::pow(w,2) - bc*std::pow(w,3) + 4*std::pow(w,4)))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[6] = (std::pow(ab,3)*(4 + std::pow(bc,2))*w + std::pow(ab,4)*(std::pow(bc,2) - 2*bc*w + 4*uc*w) + std::pow(ab,2)*(std::pow(bc,2)*(3 + std::pow(bc,2)) - 2*(bc + std::pow(bc,2)*(bc + ua - 2*uc) + 2*(ua + ub - 2*uc))*w + (4 + 3*std::pow(bc,2))*std::pow(w,2) - 4*(bc - 2*uc)*std::pow(w,3)) + 2*w*(std::pow(bc,2)*(-2*ua - bc*(1 + bc*ua) + ub + uc) + std::pow(bc,2)*(2 + std::pow(bc,2))*w - (bc + std::pow(bc,2)*(bc + ua - 2*uc) + 2*(ua + ub - 2*uc))*std::pow(w,2) + (2 + std::pow(bc,2))*std::pow(w,3) - (bc - 2*uc)*std::pow(w,4)) + ab*w*(std::pow(bc,4) + 4*std::pow(w,2) + std::pow(bc,2)*(1 + std::pow(w,2))))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[7] = 0.5;
p[8] = (3*std::pow(ab,2)*bc*(ab + bc - ua - ub + (2 + std::pow(ab,2) + std::pow(bc,2))*uc) - std::pow(ab,2)*bc*(std::pow(ab,2) + std::pow(bc,2))*w + (2*std::pow(ab,4) - std::pow(ab,3)*bc - ab*bc*(-3 + std::pow(bc,2)) + std::pow(ab,2)*(6 + 2*bc*(bc + ua) + 7*bc*uc) + 2*bc*(-3*ub + 3*uc + bc*(3 + bc*(ua + 2*uc))))*std::pow(w,2) - bc*(3*std::pow(ab,2) + 2*std::pow(bc,2))*std::pow(w,3) + (6 + 4*std::pow(ab,2) - ab*bc + 2*bc*(bc + ua + 2*uc))*std::pow(w,4) - 2*bc*std::pow(w,5) + 2*std::pow(w,6))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(bc,2) + 4*(std::pow(ab,2) + std::pow(bc,2))*std::pow(w,2) + 4*std::pow(w,4)));
}


// {Nabc -> w, Nab -> ab, Nca -> ca, Nbc -> 0, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1110(double p[9], double w, double ab, double ca, double, double ua, double ub, double uc) {
p[0] = (std::pow(ab,2)*std::pow(ca,2)*(3 + std::pow(ab,2) + std::pow(ca,2)) + (-2*std::pow(ab,3)*(1 + std::pow(ca,2)) + ab*(std::pow(ca,2) + std::pow(ca,4)) + std::pow(ab,4)*(ca - 2*uc) + 2*std::pow(ca,2)*(ua - 2*ub - ca*(1 + ca*ub) + uc) + std::pow(ab,2)*(ca + 2*(ua + ub - 2*uc) + std::pow(ca,2)*(-2*ca + 3*ua + ub + uc)))*w + 2*(std::pow(ab,4) + 2*std::pow(ca,2) + std::pow(ca,4) + std::pow(ab,2)*(2 + std::pow(ca,2)))*std::pow(w,2) - (2*std::pow(ab,3) + ab*(2 + std::pow(ca,2)) + std::pow(ab,2)*(ca - 4*ua + 2*uc) + 2*(ca + std::pow(ca,2)*(ca - 2*ua + ub) + 2*(-2*ua + ub + uc)))*std::pow(w,3) + 2*(2 + std::pow(ab,2) + std::pow(ca,2))*std::pow(w,4) - 2*(ab + ca - 2*ua)*std::pow(w,5))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[1] = (std::pow(ab,2)*(std::pow(ca,2) + std::pow(w,2))*(6 + 3*std::pow(ca,2) + 2*std::pow(w,2)) + std::pow(ab,3)*(3*std::pow(ca,2)*(-ca + ua + uc) - 2*std::pow(ca,2)*w + (-3*ca + 4*ua + 2*uc)*std::pow(w,2) - 2*std::pow(w,3)) + (std::pow(ca,2) + std::pow(w,2))*(3*(2 + std::pow(ca,2))*ub*(ca - ua - ub - uc) + 3*(2 + std::pow(ca,2))*ub*w + 2*(3 + std::pow(ca,2))*std::pow(w,2) + 2*std::pow(w,4)) + ab*(3*std::pow(ca,2)*(-ca + ua + uc) + std::pow(ca,4)*w + (6*ua + ca*(-3 + ca*(-3*ca + 4*ua + 2*uc)))*std::pow(w,2) - std::pow(ca,2)*std::pow(w,3) + (-3*ca + 4*ua + 2*uc)*std::pow(w,4) - 2*std::pow(w,5)))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[2] = (-3*std::pow(ab,3)*ca*(1 + std::pow(ca,2) + std::pow(w,2)) - 3*ab*ca*std::pow(w,2)*(1 + std::pow(ca,2) + std::pow(w,2)) + std::pow(ab,4)*(3*ca*(ca - uc) + ca*w + 2*std::pow(w,2)) + 2*std::pow(w,2)*(ca*(ca*(3 + ca*(2*ua + ub)) + 3*(ua - uc)) - std::pow(ca,3)*w + (3 + ca*(ca + 2*ua + ub))*std::pow(w,2) - ca*std::pow(w,3) + std::pow(w,4)) + std::pow(ab,2)*(3*ca*(ua + ub + ca*(2 + ca*(ua + ub)) - 2*uc) - 2*std::pow(ca,3)*w + (6 + ca*(5*ca + 4*ua + 2*ub - 3*uc))*std::pow(w,2) - ca*std::pow(w,3) + 4*std::pow(w,4)))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[3] = (-2*std::pow(ab,3)*w*(1 + std::pow(ca,2) + std::pow(w,2)) - 2*ab*w*(std::pow(ca,2) + std::pow(w,2))*(1 + std::pow(ca,2) + std::pow(w,2)) + std::pow(ab,4)*(std::pow(ca,2) + ca*w + 2*w*(-uc + w)) + 4*w*(std::pow(ca,2) + std::pow(w,2))*(ca - ua + std::pow(ca,2)*ub - uc + w + ub*(2 + std::pow(w,2))) + std::pow(ab,2)*(std::pow(ca,4) + std::pow(ca,3)*w + ca*(w + std::pow(w,3)) + std::pow(ca,2)*(3 + w*(4*ub - 2*uc + 3*w)) + 2*w*(ua + ub + 2*ub*std::pow(w,2) - (uc - w)*(2 + std::pow(w,2)))))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[4] = (3*ab*std::pow(ca,2)*(ab + ca - ua + 2*ub + (std::pow(ab,2) + std::pow(ca,2))*ub - uc) - ab*std::pow(ca,2)*(std::pow(ab,2) + std::pow(ca,2))*w + (2*std::pow(ab,2)*(3 + std::pow(ca,2)) + 2*std::pow(ca,2)*(3 + std::pow(ca,2)) + std::pow(ab,3)*(-ca + 4*ub + 2*uc) + ab*(-6*ua + 6*ub + ca*(3 + ca*(-ca + 7*ub + 2*uc))))*std::pow(w,2) - ab*(2*std::pow(ab,2) + 3*std::pow(ca,2))*std::pow(w,3) + (6 + 2*std::pow(ab,2) + 4*std::pow(ca,2) + ab*(-ca + 4*ub + 2*uc))*std::pow(w,4) - 2*ab*std::pow(w,5) + 2*std::pow(w,6))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[5] = 0.5;
p[6] = (std::pow(ab,3)*(4 + std::pow(ca,2))*w + std::pow(ab,4)*(std::pow(ca,2) - 2*ca*w + 4*uc*w) + std::pow(ab,2)*(std::pow(ca,2)*(3 + std::pow(ca,2)) - 2*(ca + std::pow(ca,2)*(ca + ub - 2*uc) + 2*(ua + ub - 2*uc))*w + (4 + 3*std::pow(ca,2))*std::pow(w,2) - 4*(ca - 2*uc)*std::pow(w,3)) + 2*w*(std::pow(ca,2)*(ua - 2*ub - ca*(1 + ca*ub) + uc) + std::pow(ca,2)*(2 + std::pow(ca,2))*w - (ca + std::pow(ca,2)*(ca + ub - 2*uc) + 2*(ua + ub - 2*uc))*std::pow(w,2) + (2 + std::pow(ca,2))*std::pow(w,3) - (ca - 2*uc)*std::pow(w,4)) + ab*w*(std::pow(ca,4) + 4*std::pow(w,2) + std::pow(ca,2)*(1 + std::pow(w,2))))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[7] = (3*std::pow(ab,2)*ca*(ab + ca - ua - ub + (2 + std::pow(ab,2) + std::pow(ca,2))*uc) - std::pow(ab,2)*ca*(std::pow(ab,2) + std::pow(ca,2))*w + (2*std::pow(ab,4) - std::pow(ab,3)*ca - ab*ca*(-3 + std::pow(ca,2)) + std::pow(ab,2)*(6 + 2*ca*(ca + ub) + 7*ca*uc) + 2*ca*(-3*ua + 3*uc + ca*(3 + ca*(ub + 2*uc))))*std::pow(w,2) - ca*(3*std::pow(ab,2) + 2*std::pow(ca,2))*std::pow(w,3) + (6 + 4*std::pow(ab,2) - ab*ca + 2*ca*(ca + ub + 2*uc))*std::pow(w,4) - 2*ca*std::pow(w,5) + 2*std::pow(w,6))/((3 + std::pow(ab,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(ab,2)*std::pow(ca,2) + 4*(std::pow(ab,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[8] = 0.5;
}


// {Nabc -> w, Nab -> ab, Nca -> ca, Nbc -> bc, aNu -> ua, bNu -> ub, cNu -> uc}
static void prb1111(double p[9], double w, double ab, double ca, double bc, double ua, double ub, double uc) {
p[0] = (std::pow(bc,3)*(4 + std::pow(ca,2))*w + std::pow(bc,4)*(std::pow(ca,2) - 2*ca*w + 4*ua*w) - 2*std::pow(ab,3)*w*(1 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2)) - ab*w*(1 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(2*std::pow(bc,2) - std::pow(ca,2) + 2*std::pow(w,2)) + std::pow(ab,4)*(std::pow(bc,2) + std::pow(ca,2) + (bc + ca - 2*uc)*w + 2*std::pow(w,2)) + std::pow(bc,2)*(std::pow(ca,2)*(3 + std::pow(ca,2)) - 2*(ca + std::pow(ca,2)*(ca - 2*ua + ub) + 2*(-2*ua + ub + uc))*w + (4 + 3*std::pow(ca,2))*std::pow(w,2) - 4*(ca - 2*ua)*std::pow(w,3)) + 2*w*(std::pow(ca,2)*(ua - 2*ub - ca*(1 + ca*ub) + uc) + std::pow(ca,2)*(2 + std::pow(ca,2))*w - (ca + std::pow(ca,2)*(ca - 2*ua + ub) + 2*(-2*ua + ub + uc))*std::pow(w,2) + (2 + std::pow(ca,2))*std::pow(w,3) - (ca - 2*ua)*std::pow(w,4)) + bc*w*(std::pow(ca,4) + 4*std::pow(w,2) + std::pow(ca,2)*(1 + std::pow(w,2))) + std::pow(ab,2)*(std::pow(bc,4) + std::pow(ca,4) + std::pow(bc,3)*w - 2*std::pow(ca,3)*w + ca*(w - std::pow(w,3)) + bc*(w - std::pow(ca,2)*w + std::pow(w,3)) + std::pow(ca,2)*(3 + w*(3*ua + ub + uc + 2*w)) + std::pow(bc,2)*(3 + 3*std::pow(ca,2) - ca*w + w*(4*ua - 2*uc + 3*w)) + 2*w*(ua + ub + 2*ua*std::pow(w,2) - (uc - w)*(2 + std::pow(w,2)))))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[1] = (3*std::pow(bc,3)*(2 + std::pow(ca,2))*ub + 6*bc*(2 + std::pow(ca,2))*ub*(std::pow(ca,2) + std::pow(w,2)) + std::pow(bc,4)*(3*std::pow(ca,2) + 4*std::pow(w,2)) - std::pow(ab,3)*(3*bc*std::pow(ca,2) + 3*std::pow(bc,2)*(ca - 2*ua) + 6*std::pow(ca,2)*(ca - ua - uc) + 2*(std::pow(bc,2) + 2*std::pow(ca,2))*w + 2*(bc + 3*ca - 4*ua - 2*uc)*std::pow(w,2) + 4*std::pow(w,3)) + 2*(std::pow(ca,2) + std::pow(w,2))*(3*(2 + std::pow(ca,2))*ub*(ca - ua - ub - uc) + 3*(2 + std::pow(ca,2))*ub*w + 2*(3 + std::pow(ca,2))*std::pow(w,2) + 2*std::pow(w,4)) + ab*(3*(2*std::pow(bc,3) + bc*(std::pow(ca,2) + std::pow(ca,4)) - std::pow(bc,4)*(ca - 2*ua) + 2*std::pow(ca,2)*(-ca + ua + uc) + std::pow(bc,2)*(4*ua - 2*uc + ca*(-1 + ca*(-2*ca + 3*ua + uc)))) + (-2*std::pow(bc,4) - 3*std::pow(bc,2)*std::pow(ca,2) + 2*std::pow(ca,4))*w + (-2*std::pow(bc,3) + bc*(6 + std::pow(ca,2)) + 12*ua + std::pow(bc,2)*(-9*ca + 14*ua + 4*uc) + 2*ca*(-3 + ca*(-3*ca + 4*ua + 2*uc)))*std::pow(w,2) - 2*(3*std::pow(bc,2) + std::pow(ca,2))*std::pow(w,3) - 2*(bc + 3*ca - 4*ua - 2*uc)*std::pow(w,4) - 4*std::pow(w,5)) + 2*std::pow(ab,2)*(std::pow(bc,2)*(3 + 3*std::pow(ca,2) + 2*std::pow(w,2)) + (std::pow(ca,2) + std::pow(w,2))*(6 + 3*std::pow(ca,2) + 2*std::pow(w,2))) + std::pow(bc,2)*(3*std::pow(ca,4) + 6*ca*ub + 3*std::pow(ca,3)*ub - 6*ub*(ua + ub + uc) + 6*ub*w + 12*std::pow(w,2) + 8*std::pow(w,4) + std::pow(ca,2)*(9 - 3*ub*(ua + ub + uc) + 3*ub*w + 11*std::pow(w,2))))/(2.*(3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[2] = (-6*std::pow(ab,3)*ca*(1 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2)) + std::pow(ab,4)*(3*std::pow(bc,2) + 3*bc*ca + 6*ca*(ca - uc) + 2*ca*w + 4*std::pow(w,2)) + (3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*ca*(bc + ca + ua - ub - uc) + std::pow(bc,2)*ca*w + 2*(2*std::pow(bc,2) + 2*bc*ca + ca*(3*ca + ua - ub - 3*uc))*std::pow(w,2) + 2*ca*std::pow(w,3) + 4*std::pow(w,4)) + std::pow(ab,2)*(3*std::pow(bc,4) + 6*ca*(ua + ub + ca*(2 + ca*(ua + ub)) - 2*uc) - 4*std::pow(ca,3)*w + 2*(6 + ca*(5*ca + 4*ua + 2*ub - 3*uc))*std::pow(w,2) - 2*ca*std::pow(w,3) + 8*std::pow(w,4) + bc*ca*(3 - 3*std::pow(ca,2) + std::pow(w,2)) + std::pow(bc,2)*(9 + 3*ca*(2*ca + 3*ua + ub - uc) - 3*ca*w + 11*std::pow(w,2))))/(2.*(3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[3] = (-2*std::pow(bc,3)*w*(1 + std::pow(ca,2) + std::pow(w,2)) - 2*bc*w*(std::pow(ca,2) + std::pow(w,2))*(1 + std::pow(ca,2) + std::pow(w,2)) - 2*std::pow(ab,3)*w*(1 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2)) + std::pow(ab,4)*(std::pow(bc,2) + std::pow(ca,2) + (bc + ca - 2*uc)*w + 2*std::pow(w,2)) + std::pow(bc,4)*(std::pow(ca,2) + ca*w + 2*w*(-ua + w)) + 4*w*(std::pow(ca,2) + std::pow(w,2))*(ca - ua + std::pow(ca,2)*ub - uc + w + ub*(2 + std::pow(w,2))) + ab*w*(1 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(std::pow(bc,2) - 2*(std::pow(ca,2) + std::pow(w,2))) + std::pow(bc,2)*(std::pow(ca,4) + std::pow(ca,3)*w + ca*(w + std::pow(w,3)) + std::pow(ca,2)*(3 + w*(-2*ua + 4*ub + 3*w)) + 2*w*(ub + uc + 2*w + 2*ub*std::pow(w,2) + std::pow(w,3) - ua*(2 + std::pow(w,2)))) + std::pow(ab,2)*(std::pow(bc,4) + std::pow(ca,4) - 2*std::pow(bc,3)*w + std::pow(ca,3)*w - bc*w*(-1 + std::pow(ca,2) + std::pow(w,2)) + ca*(w + std::pow(w,3)) + std::pow(bc,2)*(3 + 3*std::pow(ca,2) - ca*w + w*(ua + 3*ub + uc + 2*w)) + std::pow(ca,2)*(3 + w*(4*ub - 2*uc + 3*w)) + 2*w*(ua + ub + 2*ub*std::pow(w,2) - (uc - w)*(2 + std::pow(w,2)))))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[4] = (6*std::pow(bc,5)*ua + 6*std::pow(ca,2)*ua*(ca - ua - ub - uc) + 6*std::pow(ca,2)*ua*w + 4*(3*std::pow(ca,2) + std::pow(ca,4) + 3*ca*ua - 3*ua*(ua + ub + uc))*std::pow(w,2) + 12*ua*std::pow(w,3) + 4*(3 + 2*std::pow(ca,2))*std::pow(w,4) + 4*std::pow(w,6) + 6*bc*ua*(std::pow(ca,2) + 2*std::pow(w,2)) + 3*std::pow(bc,3)*ua*(4 + std::pow(ca,2) + 2*std::pow(w,2)) + std::pow(bc,4)*(3*std::pow(ca,2) + 6*ca*ua - 6*ua*(ua + ub + uc) + 6*ua*w + 4*std::pow(w,2)) - std::pow(ab,3)*(6*std::pow(bc,3) + 3*bc*std::pow(ca,2) - 6*std::pow(ca,2)*ub + 3*std::pow(bc,2)*(ca - 2*(ub + uc)) + 2*(2*std::pow(bc,2) + std::pow(ca,2))*w + 2*(3*bc + ca - 4*ub - 2*uc)*std::pow(w,2) + 4*std::pow(w,3)) + 2*std::pow(ab,2)*(3*(std::pow(bc,4) + std::pow(ca,2) + std::pow(bc,2)*(2 + std::pow(ca,2))) + (6 + 5*std::pow(bc,2) + 2*std::pow(ca,2))*std::pow(w,2) + 2*std::pow(w,4)) + std::pow(bc,2)*(3*(std::pow(ca,4) + 4*ca*ua + std::pow(ca,3)*ua - 4*ua*(ua + ub + uc) - std::pow(ca,2)*(-3 + ua*(ua + ub + uc))) + 3*(4 + std::pow(ca,2))*ua*w + (12 + 11*std::pow(ca,2) + 6*ca*ua - 6*ua*(ua + ub + uc))*std::pow(w,2) + 6*ua*std::pow(w,3) + 8*std::pow(w,4)) + ab*(6*std::pow(ca,2)*(ca + 2*ub + std::pow(ca,2)*ub - uc) - 2*std::pow(ca,4)*w + 2*(6*ub + ca*(3 + ca*(-ca + 7*ub + 2*uc)))*std::pow(w,2) - 6*std::pow(ca,2)*std::pow(w,3) - 2*(ca - 4*ub - 2*uc)*std::pow(w,4) - 4*std::pow(w,5) + std::pow(bc,4)*(3*ca + 2*w) - 6*std::pow(bc,3)*(1 + std::pow(ca,2) + std::pow(w,2)) - 3*bc*(1 + std::pow(ca,2) + std::pow(w,2))*(std::pow(ca,2) + 2*std::pow(w,2)) + std::pow(bc,2)*(6*(ub + uc) + 3*ca*(1 + 3*ca*ub + ca*uc) - 3*std::pow(ca,2)*w + (ca + 8*ub + 4*uc)*std::pow(w,2) - 2*std::pow(w,3))))/(2.*(3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[5] = (-6*std::pow(ab,3)*bc*(1 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2)) + (3 + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*bc*std::pow(ca,2)*(bc + ca - ua + ub - uc) + bc*std::pow(ca,2)*w + 2*(3*std::pow(bc,2) + 2*std::pow(ca,2) + bc*(2*ca - ua + ub - 3*uc))*std::pow(w,2) + 2*bc*std::pow(w,3) + 4*std::pow(w,4)) + std::pow(ab,4)*(6*std::pow(bc,2) + 3*std::pow(ca,2) + 4*std::pow(w,2) + bc*(3*ca - 6*uc + 2*w)) + std::pow(ab,2)*(3*std::pow(ca,2)*(3 + std::pow(ca,2)) + std::pow(bc,3)*(-3*ca + 6*(ua + ub) - 4*w) + (12 + 11*std::pow(ca,2))*std::pow(w,2) + 8*std::pow(w,4) + 2*std::pow(bc,2)*(6 + 3*std::pow(ca,2) + 5*std::pow(w,2)) + bc*(3*ca*(1 + ca*(ua + 3*ub - uc)) + 6*(ua + ub - 2*uc) - 3*std::pow(ca,2)*w + (ca + 4*ua + 8*ub - 6*uc)*std::pow(w,2) - 2*std::pow(w,3))))/(2.*(3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[6] = (std::pow(bc,2)*std::pow(ca,2)*(3 + std::pow(bc,2) + std::pow(ca,2)) + std::pow(ab,3)*(4 + std::pow(bc,2) + std::pow(ca,2))*w + (-2*std::pow(bc,3)*(1 + std::pow(ca,2)) + bc*(std::pow(ca,2) + std::pow(ca,4)) + std::pow(bc,4)*(ca - 2*ua) - 2*std::pow(ca,2)*(ca - ua + 2*ub + std::pow(ca,2)*ub - uc) + std::pow(bc,2)*(ca + 2*(-2*ua + ub + uc) + std::pow(ca,2)*(-2*ca + ua + ub + 3*uc)))*w + 2*(std::pow(bc,4) + 2*std::pow(ca,2) + std::pow(ca,4) + std::pow(bc,2)*(2 + std::pow(ca,2)))*std::pow(w,2) - (2*std::pow(bc,3) + bc*(2 + std::pow(ca,2)) + 2*(ca + std::pow(ca,2)*(ca + ub - 2*uc) + 2*(ua + ub - 2*uc)) + std::pow(bc,2)*(ca + 2*ua - 4*uc))*std::pow(w,3) + 2*(2 + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,4) - 2*(bc + ca - 2*uc)*std::pow(w,5) + std::pow(ab,4)*(std::pow(bc,2) + std::pow(ca,2) - 2*(bc + ca - 2*uc)*w) + ab*w*(std::pow(bc,2) + std::pow(bc,4) + std::pow(ca,2) - std::pow(bc,2)*std::pow(ca,2) + std::pow(ca,4) + (4 + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2)) + std::pow(ab,2)*(3*std::pow(bc,2) + std::pow(bc,4) + 3*std::pow(ca,2) + 3*std::pow(bc,2)*std::pow(ca,2) + std::pow(ca,4) - (2*std::pow(bc,3) + bc*(2 + std::pow(ca,2)) + 2*(ca + std::pow(ca,2)*(ca + ub - 2*uc) + 2*(ua + ub - 2*uc)) + std::pow(bc,2)*(ca + 2*ua - 4*uc))*w + (4 + 3*std::pow(bc,2) + 3*std::pow(ca,2))*std::pow(w,2) - 4*(bc + ca - 2*uc)*std::pow(w,3)))/((3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[7] = (-2*std::pow(ab,3)*ca*(-3 + std::pow(w,2)) + std::pow(ab,4)*(3*std::pow(bc,2) - 3*bc*ca + 6*ca*uc - 2*ca*w + 4*std::pow(w,2)) + ab*ca*(3*std::pow(bc,4) + std::pow(bc,2)*(3 - 3*std::pow(ca,2) + std::pow(w,2)) - 2*std::pow(w,2)*(-3 + std::pow(ca,2) + std::pow(w,2))) + std::pow(ab,2)*(3*std::pow(bc,4) - 6*std::pow(bc,3)*ca + 6*ca*(ca - ua - ub + (2 + std::pow(ca,2))*uc) - 2*std::pow(ca,3)*w + 2*(6 + 2*ca*(ca + ub) + 7*ca*uc)*std::pow(w,2) - 6*ca*std::pow(w,3) + 8*std::pow(w,4) + std::pow(bc,2)*(9 + 3*ca*(2*ca - ua + ub + 3*uc) - 3*ca*w + 11*std::pow(w,2)) - 3*bc*(ca + std::pow(ca,3) + 3*ca*std::pow(w,2))) + 2*(-3*std::pow(bc,3)*ca*(1 + std::pow(ca,2) + std::pow(w,2)) - 3*bc*ca*std::pow(w,2)*(1 + std::pow(ca,2) + std::pow(w,2)) + std::pow(bc,4)*(3*ca*(ca - ua) + ca*w + 2*std::pow(w,2)) + 2*std::pow(w,2)*(ca*(-3*ua + 3*uc + ca*(3 + ca*ub + 2*ca*uc)) - std::pow(ca,3)*w + (3 + ca*(ca + ub + 2*uc))*std::pow(w,2) - ca*std::pow(w,3) + std::pow(w,4)) + std::pow(bc,2)*(3*ca*(-2*ua + ub + uc + ca*(2 + ca*(ub + uc))) - 2*std::pow(ca,3)*w + (6 + ca*(5*ca - 3*ua + 2*ub + 4*uc))*std::pow(w,2) - ca*std::pow(w,3) + 4*std::pow(w,4))))/(2.*(3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
p[8] = (-2*std::pow(ab,3)*bc*(-3 + std::pow(w,2)) + std::pow(ab,4)*(3*std::pow(ca,2) + bc*(-3*ca + 6*uc - 2*w) + 4*std::pow(w,2)) + ab*bc*(3*std::pow(ca,4) + std::pow(ca,2)*(3 - 3*std::pow(bc,2) + std::pow(w,2)) - 2*std::pow(w,2)*(-3 + std::pow(bc,2) + std::pow(w,2))) + 2*(2*std::pow(w,2)*(std::pow(ca,2) + std::pow(w,2))*(3 + std::pow(ca,2) + std::pow(w,2)) + std::pow(bc,2)*(std::pow(ca,2) + std::pow(w,2))*(6 + 3*std::pow(ca,2) + 2*std::pow(w,2)) + std::pow(bc,3)*(3*std::pow(ca,2)*(-ca + ua + uc) - 2*std::pow(ca,2)*w + (-3*ca + 2*ua + 4*uc)*std::pow(w,2) - 2*std::pow(w,3)) + bc*(-3*std::pow(ca,2)*(ca - ua + 2*ub + std::pow(ca,2)*ub - uc) + std::pow(ca,4)*w + (-6*ub + 6*uc + ca*(-3 + ca*(-3*ca + 2*ua - 3*ub + 4*uc)))*std::pow(w,2) - std::pow(ca,2)*std::pow(w,3) + (-3*ca + 2*ua + 4*uc)*std::pow(w,4) - 2*std::pow(w,5))) + std::pow(ab,2)*(3*std::pow(ca,2)*(3 + std::pow(ca,2)) + std::pow(bc,3)*(-3*ca + 6*uc - 2*w) + (12 + 11*std::pow(ca,2))*std::pow(w,2) + 8*std::pow(w,4) + std::pow(bc,2)*(6 + 6*std::pow(ca,2) + 4*std::pow(w,2)) - bc*(6*std::pow(ca,3) + 6*(ua + ub - 2*uc) + 3*std::pow(ca,2)*(-ua + ub - 3*uc + w) + 2*std::pow(w,2)*(-2*ua - 7*uc + 3*w) + ca*(3 + 9*std::pow(w,2)))))/(2.*(3 + std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2) + std::pow(w,2))*(3*std::pow(bc,2)*std::pow(ca,2) + 3*std::pow(ab,2)*(std::pow(bc,2) + std::pow(ca,2)) + 4*(std::pow(ab,2) + std::pow(bc,2) + std::pow(ca,2))*std::pow(w,2) + 4*std::pow(w,4)));
}


void SoundAgent::updateProbs()
{
	// a_abc a_ab a_ca b_abc b_ab b_bc c_abc c_ca c_bc

	auto &Nabc = Habc.nbr_cards;
	auto &Nab = Hab.nbr_cards;
	auto &Nca = Hca.nbr_cards;
	auto &Nbc = Hbc.nbr_cards;

	LOG("Nabc: " << Nabc << ", Nab: " << Nab << ", Nca: " << Nca << ", Nbc: " << Nbc);
	LOG("Nu_a: " << Nu_a << ", Nu_b: " << Nu_b << ", Nu_c: " << Nu_c);

	double prb[9] = {0};

	if(Nabc > 0)
		if(Nab > 0)
			if(Nca > 0)
				if (Nbc > 0)
					prb1111(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb1110(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
			else
				if (Nbc > 0)
					prb1101(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb1100(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
		else
			if(Nca > 0)
				if (Nbc > 0)
					prb1011(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb1010(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
			else
				if (Nbc > 0)
					prb1001(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb1000(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
	else
		if(Nab > 0)
			if(Nca > 0)
				if (Nbc > 0)
					prb0111(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb0110(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
			else
				if (Nbc > 0)
					prb0101(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb0100(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
		else
			if(Nca > 0)
				if (Nbc > 0)
					prb0011(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb0010(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
			else
				if (Nbc > 0)
					prb0001(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);
				else
					prb0000(prb, Nabc, Nab, Nca, Nbc, Nu_a, Nu_b, Nu_c);

	LOG("updateProbs: prb: " << arr_toString(prb, 9));

	Pa.clear();
	Pb.clear();
	Pc.clear();
	Pa.update(Habc, prb[0]);
	Pa.update(Hab, prb[1]);
	Pa.update(Hca, prb[2]);
	Pb.update(Habc, prb[3]);
	Pb.update(Hab, prb[4]);
	Pb.update(Hbc, prb[5]);
	Pc.update(Habc, prb[6]);
	Pc.update(Hca, prb[7]);
	Pc.update(Hbc, prb[8]);
	Pa.update(Ha, 1.0);
	Pb.update(Hb, 1.0);
	Pc.update(Hc, 1.0);
	const Hand &hna = Habc.uni(Hab).uni(Hca).uni(Ha).cmpl();
	const Hand &hnb = Habc.uni(Hab).uni(Hbc).uni(Hb).cmpl();
	const Hand &hnc = Habc.uni(Hbc).uni(Hca).uni(Hc).cmpl();
	Pa.update(hna, 0.0);
	Pb.update(hnb, 0.0);
	Pc.update(hnc, 0.0);
}


Card SoundAgent::act(const State &state, const History &hist)
{
	int op_team = (team_id + 1) % Hokm::N_TEAMS;
	LOG(
		"--- " << name << " pl_id " << player_id << " team " << team_id
			   << " ord " << (int)state.ord
			   << " last_ord " << last_ord << " last_led "
			   << Card::SU_STR[last_led] << " led " << Card::SU_STR[state.led]
			   << " trump " << Card::SU_STR[state.trump] << " trick_id " << state.trick_id
			   << "\nhand: " << hand.to_string());

	int b_id = (player_id + 3) % 4;
	int c_id = (player_id + 2) % 4;
	int a_id = (player_id + 1) % 4;

	bool critical = state.score[op_team] >= (Hokm::RND_WIN_SCORE - 2) ||
					state.score[op_team] > state.score[team_id] + Hokm::RND_WIN_SCORE / 2;
	LOG("critical: " << critical);

	Card c;
	switch (last_ord)
	{
	case -1:
		break;
	case 0:
		c = hist.played_cards[b_id].at(-1);
		LOG("op_b " << b_id << " last table " << c.to_string());
		update_oth_hands(b_id, c, last_led);
		[[fallthrough]];
	case 1:
		c = hist.played_cards[c_id].at(-1);
		LOG("comp " << c_id << " last table " << c.to_string());
		update_oth_hands(c_id, c, last_led);
		[[fallthrough]];
	case 2:
		c = hist.played_cards[a_id].at(-1);
		LOG("op_a " << a_id << " last table " << c.to_string());
		update_oth_hands(a_id, c, last_led);
	}

	switch (state.ord)
	{
	case 3:
		c = state.table[a_id];
		LOG("op_a " << a_id << " on table " << c.to_string());
		update_oth_hands(a_id, c, state.led);
		[[fallthrough]];
	case 2:
		c = state.table[c_id];
		LOG("comp " << c_id << " on table " << c.to_string());
		update_oth_hands(c_id, c, state.led);
		[[fallthrough]];
	case 1:
		c = state.table[b_id];
		LOG("op_b " << b_id << " on table " << c.to_string());
		update_oth_hands(b_id, c, state.led);
	}
	Ncards_a = Ncards_b = Ncards_c = Hokm::N_DELT - state.trick_id;
	switch (state.ord)
	{
	case 3:
		Ncards_a--;
		[[fallthrough]];
	case 2:
		Ncards_c--;
		[[fallthrough]];
	case 1:
		Ncards_b--;
	}
	updateHandsNcards();
	Nu_a = Ncards_a - Ha.nbr_cards;
	Nu_b = Ncards_b - Hb.nbr_cards;
	Nu_c = Ncards_c - Hc.nbr_cards;
	LOG("Habc: " << Habc.to_string());
	LOG("Hab: " << Hab.to_string());
	LOG("Hca: " << Hca.to_string());
	LOG("Hbc: " << Hbc.to_string());
	LOG("Ha: " << Ha.to_string());
	LOG("Hb: " << Hb.to_string());
	LOG("Hc: " << Hc.to_string());
	updateProbs();
	LOG("Pa.nbr: " << Pa.nbr() << ", Pb.nbr: " << Pb.nbr() << ", Pc.nbr: " << Pc.nbr());
	LOG("Pa: " << Pa.to_string());
	LOG("Pb: " << Pb.to_string());
	LOG("Pc: " << Pc.to_string());
#ifdef DEBUG
	if (Pa.nbr() != Ncards_a || Pb.nbr() != Ncards_b || Pc.nbr() != Ncards_c)
	{
		LOG("Ncards_a: " << Ncards_a << " Ncards_b: " << Ncards_b << " Ncards_c: " << Ncards_c);
		throw std::runtime_error("Nbr of cards inconsistencies!");
	}
#endif

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
		Hand ps_hi = hand.lead_gt(H_ab, Card::NON_SU, state.trump);
		LOG("pssbl_hi_hand: " << ps_hi.to_string());
		if (ps_hi.nbr_cards == 0)
		{
			out = hand.min_mil_trl(Card::NON_SU, state.trump);
			break;
		}
		Hand ps_ab = Habc.uni(Hca).uni(Hbc);
		Hand ps_play;
		Card c;
		for (Suit s = 0; s < Card::N_SUITS; s++)
		{
			for (int i = 0; i < ps_hi.len[s]; i++)
			{
				c.set(s, ps_hi.cards[s][i]);
				Hand ps_abc_hi = ps_ab.gt_c_led(c, state.led, state.trump);
				double pr_na = Pa.probNotIn(ps_abc_hi);
				double pr_nb = Pb.probNotIn(ps_abc_hi);
				LOG("Card " << c.to_string() << ", prob: " << pr_na * pr_nb);
				if (pr_na * pr_nb > this->prob_floor)
					ps_play.add(c);
			}
		}
		if (ps_play.nbr_cards == 0)
		{
			out = hand.min_mil_trl(Card::NON_SU, state.trump);
			break;
		}
		out = ps_play.min_mil_trl(state.led, state.trump);
	}
	break;

	case 1:
	{
		LOG("case 1:");
		assert(a_card == Card::NONE && b_card != Card::NONE && c_card == Card::NONE);

		Hand H_a = Ha;
		H_a.add(b_card);
		Hand ps_hi = hand.lead_gt(H_a, state.led, state.trump);
		LOG("pssbl_hi_hand: " << ps_hi.to_string());
		if (ps_hi.nbr_cards == 0)
		{
			out = hand.min_mil_trl(state.led, state.trump);
			break;
		}
		Hand ps_play;
		Hand ps_a = Habc.uni(Hca).uni(Hab);
		Card c;
		for (Suit s = 0; s < Card::N_SUITS; s++)
		{
			for (int i = 0; i < ps_hi.len[s]; i++)
			{
				c.set(s, ps_hi.cards[s][i]);
				Hand ps_ac_hi = ps_a.gt_c_led(c, state.led, state.trump);
				double pr_na = Pa.probNotIn(ps_ac_hi);
				LOG("Card " << c.to_string() << ", prob: " << pr_na);
				if (pr_na > this->prob_floor)
					ps_play.add(c);
			}
		}
		if (ps_play.nbr_cards == 0)
		{
			out = hand.min_mil_trl(state.led, state.trump);
			break;
		}
		out = ps_play.min_mil_trl(state.led, state.trump);
	}
	break;

	case 2:
	{
		LOG("case 2:");
		assert(a_card == Card::NONE && b_card != Card::NONE && c_card != Card::NONE);

		Hand H_a = Ha;
		H_a.add(b_card);
		Hand ps_hi = hand.lead_gt(H_a, state.led, state.trump);
		LOG("pssbl_hi_hand: " << ps_hi.to_string());
		if (ps_hi.nbr_cards == 0)
		{
			out = hand.min_mil_trl(state.led, state.trump);
			break;
		}
		Hand ps_a = Habc.uni(Hca).uni(Hab);
		Hand ps_comp_ac_hi = ps_a.gt_c_led(c_card, state.led, state.trump);
		double pr_comp_na = Pa.probNotIn(ps_comp_ac_hi);
		LOG("comp c " << c_card.to_string() << ", prob: " << pr_comp_na);
		if (!critical && pr_comp_na > this->prob_floor)
		{
			out = hand.min_mil_trl(state.led, state.trump);
			break;
		}
		Hand ps_play;
		Card c;
		for (Suit s = 0; s < Card::N_SUITS; s++)
		{
			for (int i = 0; i < ps_hi.len[s]; i++)
			{
				c.set(s, ps_hi.cards[s][i]);
				Hand ps_ac_hi = ps_a.gt_c_led(c, state.led, state.trump);
				double pr_na = Pa.probNotIn(ps_ac_hi);
				LOG("Card " << c.to_string() << ", prob: " << pr_na);
				if (pr_na > this->prob_floor)
					ps_play.add(c);
			}
		}
		if (ps_play.nbr_cards == 0)
		{
			out = hand.min_mil_trl(state.led, state.trump);
			break;
		}
		out = ps_play.min_mil_trl(state.led, state.trump);
		H_a.add(ps_a);
		int dfn = H_a.diff_between(out, c_card, state.trump);
		LOG("diff " << c_card.to_string() << "-" << out.to_string() << ": " << dfn);
		if (dfn >= 0)
		{
			LOG("Companion's card is good enough.");
			out = hand.min_mil_trl(state.led, state.trump);
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
			out = hand.min_mil_trl(state.led, state.trump);
			break;
		}
		Hand hi_hand = hand.gt_c_led(bst_op, state.led, state.trump);
		LOG("hi_hand: " << hi_hand.to_string());
		if (hi_hand.nbr_cards)
		{
			out = hi_hand.min_mil_trl(state.led, state.trump);
		}
		else
			out = hand.min_mil_trl(state.led, state.trump);
	}
	break;
	default:
		throw std::logic_error("Shouldn't be here!");
	}
	hand.remove(out);
	LOG(
		"--- " << name << " --- out card: " << out.to_string() << " ---");
	if (last_led == Card::NON_SU)
		last_led = out.su;
	return out;
}



SoundAgent::SoundAgent(double min_prob, double trump_prb_cap, double max_prob)
    : Agent(),
      mt_rnd_gen(std::mt19937(std::random_device()())),
      prob_floor(min_prob),
	  trump_prob_cap(trump_prb_cap),
      prob_ceiling(max_prob)
{
	name = "AI_" + std::to_string(player_id);
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

	Habc.clear();
	Hab.clear();
	Hca.clear();
	Hbc.clear();
	Ha.clear();
	Hb.clear();
	Hc.clear();

	Pa.clear();
	Pb.clear();
	Pc.clear();

	last_ord = -1;
	last_led = Card::NON_SU;
}