#pragma once

#include "GameConfig.h"
#include "Card.h"
#include "Hand.h"

class ProbHand
{
    double prob[Card::N_SUITS][Card::N_RANKS]{};

public:
    double getProb(const Card& c) const;

    double total() const;
    double total(Suit s) const;
    double total(const Hand& h) const;

    int nbr() const;

    ProbHand& update(const Card& c, double prob);
    ProbHand& update(Suit s, double prob);
    ProbHand& update(const Hand& h, double prob);

    ProbHand& renormalize(double N);

    double probIn(const Hand&) const;

    double probNotIn(Suit s) const;
    double probNotIn(const Hand& filter) const;

    Hand gt(double prob) const;
    Hand lt(double prob) const;
    Hand lte(double prob) const;
    Hand lte(double prob, const Hand& filter) const;

    ProbHand& clear();

    std::string to_string() const;
};

