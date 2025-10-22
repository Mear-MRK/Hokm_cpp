/*
 * Deck.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#include "Deck.h"
#include "CardStack.h"
#include "GameConfig.h"

#include <iostream>

Deck::Deck()
    : outside(false), mt_rnd_gen(std::mt19937(std::random_device()())) {
  for (int i = 0; i < Card::N_CARDS; i++)
    cards[i] = Card(i);
}

Deck::Deck(const CardStack &out_deck)
    : outside(true), mt_rnd_gen(std::mt19937(std::random_device()())) {
  for (int i = 0; i < out_deck.get_nbr_cards(); i++)
    cards[i] = out_deck.at(i);
}

CardStack Deck::to_cardStack() const { return CardStack(cards, Card::N_CARDS); }

void Deck::shuffle_overhand(int cmplx) {
  if (cmplx < 1)
    return;

  int n = Card::N_CARDS;
  std::vector<Card> tmp(n);

  for (int it = 0; it < cmplx; ++it) {
    // probability for geometric distribution: larger cmplx -> smaller average
    // packet
    double p = 0.25 + 0.05 * cmplx;
    if (p > 0.9)
      p = 0.9;
    std::geometric_distribution<int> geo(p);

    // split into packets taken from the top, then reassemble packets in reverse
    // order
    std::vector<std::vector<Card>> packets;
    packets.reserve(n / 2 + 1);

    int pos = 0;
    while (pos < n) {
      int sz = 1 + geo(mt_rnd_gen);
      if (sz > n - pos)
        sz = n - pos;
      packets.emplace_back();
      packets.back().reserve(sz);
      for (int k = 0; k < sz; ++k)
        packets.back().push_back(cards[pos++]);
    }

    int out = 0;
    for (int pi = static_cast<int>(packets.size()) - 1; pi >= 0; --pi) {
      for (const Card &c : packets[pi])
        tmp[out++] = c;
    }

    for (int i = 0; i < n; ++i)
      cards[i] = tmp[i];
  }
}

void Deck::shuffle_riffle(int cmplx) {
  if (cmplx < 1)
    return;

  int n = Card::N_CARDS;
  int iterations = cmplx;
  std::vector<Card> tmp(n);

  for (int it = 0; it < iterations; ++it) {
    int left_end = n / 2; // cut into two halves
    int left = 0;
    int right = left_end;
    int out = 0;
    std::bernoulli_distribution coin(0.5);

    // Interleave by flipping a fair coin for each next card.
    // If one half is exhausted, take from the other.
    while (left < left_end || right < n) {
      if (left >= left_end) {
        tmp[out++] = cards[right++];
        continue;
      }
      if (right >= n) {
        tmp[out++] = cards[left++];
        continue;
      }
      if (coin(mt_rnd_gen))
        tmp[out++] = cards[left++];
      else
        tmp[out++] = cards[right++];
      // std::cout << tmp[out-1].to_string() << " ";
    }
    // std::cout << std::endl;
    // copy back
    for (int i = 0; i < n; ++i) {
      cards[i] = tmp[i];
      // std::cout << cards[i].to_string() << " ";
    }
  }
}

void Deck::shuffle_rnd() {
  for (int i = 0; i < Card::N_CARDS - 1; i++) {
    std::uniform_int_distribution<int> int_rnd_dist(i, Card::N_CARDS - 1);
    int j = int_rnd_dist(mt_rnd_gen);
    if (j != i) {
      Card tmp = cards[i];
      cards[i] = cards[j];
      cards[j] = tmp;
    }
  }
}

void Deck::shuffle(int cmplx) {
  for (int i = 0; i < cmplx; i++) {
    shuffle_overhand(1);
    shuffle_riffle(1);
  }
}

void Deck::shuffle_rnd_bag() {
  int nbr_delt = Card::N_CARDS / Hokm::N_PLAYERS;
  for (int s = 0; s < Hokm::N_PLAYERS - 1; s++) {
    std::uniform_int_distribution<int> int_rnd_dist(0, Hokm::N_PLAYERS - s - 1);
    int i_min = s * nbr_delt;
    int i_max = i_min + nbr_delt;
    for (int i = i_min; i < i_max; i++) {
      int j = int_rnd_dist(mt_rnd_gen);
      if (j) {
        int i_j = i + j * nbr_delt;
        Card tmp = cards[i];
        cards[i] = cards[i_j];
        cards[i_j] = tmp;
      }
    }
  }
}

void Deck::shuffle_deal(CardStack *stacks, int opening_player, int cmplx) {
  shuffle(cmplx);
  deal(stacks, opening_player);
}

void Deck::deal(CardStack *stacks, int opening_player) {
  Card *card_ptr = &cards[0];
  for (int i = 0; i < Hokm::N_PLAYERS; i++) {
	int k = (i + opening_player) % Hokm::N_PLAYERS;
    stacks[k].clear();
    stacks[k].append(card_ptr, 5);
    card_ptr += 5;
  }
  for (int j = 0; j < 2; j++)
    for (int i = 0; i < Hokm::N_PLAYERS; i++) {
	int k = (i + opening_player) % Hokm::N_PLAYERS;
      stacks[k].append(card_ptr, 4);
      card_ptr += 4;
    }
}
