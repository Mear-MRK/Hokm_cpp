#include "ProbHand.h"

#include <cassert>
#include <cmath>
#include <sstream>
#include <iomanip>

#include "utils.h"

ProbHand &ProbHand::update(const Hand &h, double prob)
{
    assert(0 <= prob && prob <= 1);
    if (h.nbr_cards == 0)
        return *this;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (int i = 0; i < h.len[s]; i++)
        {
            Rank r = h.cards[s][i];
            this->prob[s][r] = prob;
        }
    return *this;
}

ProbHand &ProbHand::renormalize(double N)
{
    assert(N >= 0 && N <= Card::N_CARDS);
    double sum_1 = 0, sum = 0;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
        {
            double prb = this->prob[s][r];
            if (prb == 1.0)
                sum_1 += 1;
            else
                sum += prb;
        }
    if (sum == 0)
        return *this;
    double alpha = (N - sum_1) / sum;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
        {
            double prb = this->prob[s][r];
            if (prb < 1.0)
            {
                prb *= alpha;
                if (prb > 1.0)
                    prb = 1.0;
                this->prob[s][r] = prb;
            }
        }
    return *this;
}

double ProbHand::getProb(const Card &c) const
{
    assert(c != Card::NONE);
    return prob[c.su][c.rnk];
}

ProbHand &ProbHand::update(const Card &c, double prob)
{
    if (c == Card::NONE)
        return *this;
    assert(0 <= prob && prob <= 1);
    this->prob[c.su][c.rnk] = prob;
    return *this;
}

ProbHand &ProbHand::update(Suit s, double prb)
{
    assert(0 <= s && s < Card::N_SUITS);
    assert(0 <= prb && prb <= 1);
    std::fill(&prob[s][0], &prob[s][0] + Card::N_RANKS, prb);
    return *this;
}

double ProbHand::total() const
{
    double sum = 0;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
            sum += this->prob[s][r];
    return sum;
}

double ProbHand::total(Suit s) const
{
    assert(0 <= s && s < Card::N_SUITS);
    double sum = 0;
    for (Rank r = 0; r < Card::N_RANKS; r++)
        sum += this->prob[s][r];
    return sum;
}

double ProbHand::total(const Hand &h) const
{
    double sum = 0;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (int i = 0; i < h.len[s]; i++)
        {
            Rank r = h.cards[s][i];
            sum += this->prob[s][r];
        }
    return sum;
}

int ProbHand::nbr() const
{
    return static_cast<int>(round(total()));
}

double ProbHand::probIn(const Hand &h) const
{
    double prb = 1.0;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (int i = 0; i < h.len[s]; i++)
        {
            Rank r = h.cards[s][i];
            prb *= this->prob[s][r];
        }
    return prb;
}

double ProbHand::probNotIn(Suit s) const
{
    assert(0 <= s && s < Card::N_SUITS);
    double prb = 1.0;
    for (Rank r = 0; r < Card::N_RANKS; r++)
        prb *= (1.0 - this->prob[s][r]);
    return prb;
}

double ProbHand::probNotIn(const Hand &h) const
{
    double prb = 1.0;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (int i = 0; i < h.len[s]; i++)
        {
            Rank r = h.cards[s][i];
            prb *= (1.0 - this->prob[s][r]);
        }
    return prb;
}

Hand ProbHand::gt(double prb) const
{
    // assert(0 <= prb && prb <= 1);
    Hand out;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
            if (this->prob[s][r] > prb)
                out.add(Card(s, r));
    return out;
}

Hand ProbHand::gte(double prob) const
{
    // assert(0 <= prb && prb <= 1);
    Hand out;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
            if (this->prob[s][r] >= prob)
                out.add(Card(s, r));
    return out;
}

Hand ProbHand::lt(double prb) const
{
    // assert(0 <= prb && prb <= 1);
    Hand out;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
            if (this->prob[s][r] < prb)
                out.add(Card(s, r));
    return out;
}

Hand ProbHand::lte(double prob) const
{
    Hand out;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
            if (this->prob[s][r] <= prob)
                out.add(Card(s, r));
    return out;
}

Hand ProbHand::bound(double gte_prb, double lt_prb) const
{
    // assert(lt_prb > gte_prb);
    Hand out;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (Rank r = 0; r < Card::N_RANKS; r++)
            if (this->prob[s][r] < lt_prb && this->prob[s][r] >= gte_prb)
                out.add(Card(s, r));
    return out;
}

Hand ProbHand::lte(double prob, const Hand &filter) const
{
    Hand out;
    for (Suit s = 0; s < Card::N_SUITS; s++)
        for (int i = 0; i < filter.len[s]; i++)
        {
            Rank r = filter.cards[s][i];
            if (this->prob[s][r] <= prob)
                out.add(Card(s, r));
        }
    return out;
}

ProbHand &ProbHand::clear()
{
    std::fill(&prob[0][0], &prob[0][0] + Card::N_CARDS, 0.0);
    return *this;
}

std::string ProbHand::to_string() const
{
    std::stringstream out;
    out << "  ";
    for (Rank r = 0; r < Card::N_RANKS; r++)
        out << Card::RNK_STR[r] + "     ";
    out << "\n";
    out << std::fixed << std::setprecision(3);
    for (Suit s = 0; s < Card::N_SUITS; s++)
    {
        out << Card::SU_STR[s] + ": ";
        for (Rank r = 0; r < Card::N_RANKS; r++)
            out << " " << prob[s][r];
        if (s != Card::N_SUITS - 1)
            out << "\n";
    }

    return out.str();
}
