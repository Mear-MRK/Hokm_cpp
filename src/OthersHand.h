#pragma once

#include <assert.h>

#include "Hand.h"
#include "utils.h"


struct OthersHand
{
    Hand Habc{};
    Hand Hab{};
    Hand Hbc{};
    Hand Hca{};
    Hand Ha{};
    Hand Hb{};
    Hand Hc{};

    OthersHand& clear();
};


OthersHand& OthersHand::clear()
{
    Ha.clear();
    Hb.clear();
    Hc.clear();
    Hab.clear();
    Hbc.clear();
    Hca.clear();
    Habc.clear();

    return *this;
}

static std::pair<Card, double> maxProb(const Hand& hand, const OthersHand &othsHand, int nbrCompEx, Suit led, Suit trump)
{
    // std::fill(&prob[0][0], &prob[0][0] + Card::N_CARDS, 0);
    const Hand &pssbleOpHand = othsHand.Habc.uni(othsHand.Hab).uni(othsHand.Hca).uni(othsHand.Hbc);
    const Hand &certOpHand = othsHand.Hab.uni(othsHand.Ha).uni(othsHand.Hb);
    // nbrCompEx = othsHands.
    Card c;
    double max_prb = 0;
    Card max_c;
    double prb;
    for (Suit s = 0; s < Card::N_SUITS; s++)
    {
        for (int i = 0; i < hand.len[s]; i++)
        {
            prb = 0;
            Rank r = hand.cards[s][i];
            c.set(s, r);
            if (certOpHand.gt_c_led(c, led, trump).nbr_cards == 0)
            {
                int k = pssbleOpHand.gt_c_led(c, led, trump).nbr_cards; // not exactly correct; s may all be in comp
                prb = chs_prob(pssbleOpHand.nbr_cards, k, nbrCompEx);
                if (prb > max_prb)
                {
                    max_prb = prb;
                    max_c = c;
                }
            }
            LOG("maxProb " + c.to_string() + " " << prb);
            // this->prob[s][i] = prb;
        }
    }
    return std::make_pair(max_c, max_prb);
}

// n choose k not in n_ex
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

