/*
 * SoundAgent.cpp
 *
 *  Created on: May 29, 2023
 *      Author: mear
 */

#include "SoundAgent.h"

#include <cassert>
#include <cstring>
#include <string>

#include "utils.h"

Suit SoundAgent::call_trump(const CardStack &first_5cards) {
  Hand hand = first_5cards.to_Hand();

  LOG(name << ", trump call, first5:\n" << hand.to_su_string());

  float scr[Card::N_SUITS];

  float beta = 1.0f / 6;

  for (Suit s = 0; s < Card::N_SUITS; s++) {
    float sum_r = 0;
    for (int i = 0; i < hand.len[s]; i++)
      sum_r += hand.cards[s][i];
    scr[s] = hand.len[s] + beta * sum_r;
  }

  double max_scr = -1;
  Suit trump = Card::NON_SU;
  for (Suit t = 0; t < Card::N_SUITS; t++) {
    LOG(name << ", trump call, suit " << Card::SU_STR[t]
             << ", score: " << scr[t]);
    if (scr[t] > max_scr) {
      max_scr = scr[t];
      trump = t;
    } else if (scr[t] == max_scr && hand.len[t] < hand.len[trump])
      trump = t;
  }
  LOG(name << ", alg. trump: " << Card::SU_STR[trump]);
  //	if (trump == Card::NON_SU)
  //		return first_5cards.at(
  //				std::uniform_int_distribution<int>(0,
  // 4)(mt_rnd_gen)).su;
  return trump;
}

void SoundAgent::init_round(const Hand &hand) {

  this->hand = hand;

  Habc = this->hand.cmpl();
  Ha = Hb = Hc = Hab = Hbc = Hca = Hand::EMPTY;
  Nu_a = Nu_b = Nu_c = Hokm::N_DELT;
  last_ord = -1;
  last_led = Card::NON_SU;
}

void SoundAgent::update_oth_hands(int pl_id, const Card &pl_card, Suit led) {
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

    if (!(Habc.remove(pl_card) || Hca.remove(pl_card) || Hbc.remove(pl_card))) {
      if (!Hc.remove(pl_card))
        std::cerr << "The card is nowhere! c: " << pl_card.to_string()
                  << " H: " << (Hc.uni(Hca).uni(Hbc).uni(Habc)).to_string()
                  << std::endl;
    }
    if (led != Card::NON_SU && pl_card.su != led) {
      Hab.add(Habc.pop(led));
      Hand h = Hca.pop(led);
      Ha.add(h);
      h = Hbc.pop(led);
      Hb.add(h);
    }
  } else if ((player_id + 1) % Hokm::N_PLAYERS == pl_id) // a
  {
    if (!(Habc.remove(pl_card) || Hca.remove(pl_card) || Hab.remove(pl_card))) {
      if (!Ha.remove(pl_card))
        std::cerr << "The card is nowhere! a: " << pl_card.to_string()
                  << " H: " << (Ha.uni(Hca).uni(Hab).uni(Habc)).to_string()
                  << std::endl;
    }
    if (led != Card::NON_SU && pl_card.su != led) {
      Hbc.add(Habc.pop(led));
      Hand h = Hca.pop(led);
      Hc.add(h);
      h = Hab.pop(led);
      Hb.add(h);
    }
  } else // b
  {
    if (!(Habc.remove(pl_card) || Hab.remove(pl_card) || Hbc.remove(pl_card))) {
      if (!Hb.remove(pl_card))
        std::cerr << "The card is nowhere! b: " << pl_card.to_string()
                  << " H: " << (Hb.uni(Hbc).uni(Hab).uni(Habc)).to_string()
                  << std::endl;
    }

    if (led != Card::NON_SU && pl_card.su != led) {
      Hca.add(Habc.pop(led));
      Hand h = Hbc.pop(led);
      Hc.add(h);
      h = Hab.pop(led);
      Ha.add(h);
    }
  }
}

void SoundAgent::updateHandsNcards() {
  int &Nabc = Habc.nbr_cards;
  int &Nab = Hab.nbr_cards;
  int &Nca = Hca.nbr_cards;
  int &Nbc = Hbc.nbr_cards;
  int &Na = Ha.nbr_cards;
  int &Nb = Hb.nbr_cards;
  int &Nc = Hc.nbr_cards;

  bool changed = false;
  do {
    changed = false;
    if (Ncards_a == Na && (Nabc + Nab + Nca)) {
      Hbc.add(Habc);
      Habc.clear();
      Hb.add(Hab);
      Hab.clear();
      Hc.add(Hca);
      Hca.clear();
      changed = true;
    }
    if (Ncards_b == Nb && (Nabc + Nab + Nbc)) {
      Hca.add(Habc);
      Habc.clear();
      Ha.add(Hab);
      Hab.clear();
      Hc.add(Hbc);
      Hbc.clear();
      changed = true;
    }
    if (Ncards_c == Nc && (Nabc + Nbc + Nca)) {
      Hab.add(Habc);
      Habc.clear();
      Ha.add(Hca);
      Hca.clear();
      Hb.add(Hbc);
      Hbc.clear();
      changed = true;
    }
    if (Ncards_a + Ncards_b == Na + Nb + Nab && (Nabc + Nca + Nbc)) {
      Hc.add(Habc);
      Habc.clear();
      Hc.add(Hca);
      Hca.clear();
      Hc.add(Hbc);
      Hbc.clear();
      changed = true;
    }
    if (Ncards_a + Ncards_c == Na + Nc + Nca && (Nabc + Nab + Nbc)) {
      Hb.add(Habc);
      Habc.clear();
      Hb.add(Hab);
      Hab.clear();
      Hb.add(Hbc);
      Hbc.clear();
      changed = true;
    }
    if (Ncards_b + Ncards_c == Nb + Nc + Nbc && (Nabc + Nab + Nca)) {
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

// Player indices
enum Player { A = 0, B = 1, C = 2 };

// Bit masks for allowed sets
constexpr int MASK_A = 1 << A;
constexpr int MASK_B = 1 << B;
constexpr int MASK_C = 1 << C;

// Check membership of a specific card in a Hand
inline bool inHand(const Hand &h, const Card &c) { return h.is_in(c); }

// Build the unknown universe U and per-card allowed masks from ambiguous sets
void buildUnknownUniverseAndMasks(const Hand &Habc, const Hand &Hab,
                                  const Hand &Hbc, const Hand &Hca,
                                  std::vector<Card> &U,
                                  std::vector<int> &allowedMask // out
) {
  U.clear();
  allowedMask.clear();
  U.reserve(52);
  allowedMask.reserve(52);

  for (int su = 0; su < Card::N_SUITS; ++su) {
    for (int r = 0; r < Card::N_RANKS; ++r) {
      Card c(su, r);

      int mask = 0;
      if (inHand(Hab, c))
        mask |= (MASK_A | MASK_B);
      if (inHand(Hbc, c))
        mask |= (MASK_B | MASK_C);
      if (inHand(Hca, c))
        mask |= (MASK_C | MASK_A);
      if (inHand(Habc, c))
        mask |= (MASK_A | MASK_B | MASK_C);

      if (mask != 0) {
        U.push_back(c);
        allowedMask.push_back(mask);
      }
    }
  }
}

// Forward DP over counts assigned to A and B (C is implicit)
// Returns a table dp[ca][cb] = number of ways to assign all M cards where
// A receives ca, B receives cb, C receives (M - ca - cb).
// Bounds for ca, cb are clamped to [0..t_a], [0..t_b] to limit table size.
std::vector<std::vector<long double>>
countAssignmentsDP_AB(const std::vector<int> &masksOther, int t_a, int t_b) {
  const int M = static_cast<int>(masksOther.size());
  std::vector<std::vector<long double>> dp_prev(
      t_a + 1, std::vector<long double>(t_b + 1, 0.0L));
  std::vector<std::vector<long double>> dp_next(
      t_a + 1, std::vector<long double>(t_b + 1, 0.0L));

  dp_prev[0][0] = 1.0L;

  for (int i = 0; i < M; ++i) {
    std::fill(dp_next.begin(), dp_next.end(),
              std::vector<long double>(t_b + 1, 0.0L));
    const int mask = masksOther[i];

    // ca ranges 0..min(i, t_a), cb ranges 0..min(i - ca, t_b)
    const int ca_max = std::min(i, t_a);
    for (int ca = 0; ca <= ca_max; ++ca) {
      const int cb_max = std::min(i - ca, t_b);
      for (int cb = 0; cb <= cb_max; ++cb) {
        long double ways = dp_prev[ca][cb];
        if (ways == 0.0L)
          continue;

        // Assign current card to A
        if ((mask & MASK_A) && ca + 1 <= t_a) {
          dp_next[ca + 1][cb] += ways;
        }
        // Assign to B
        if ((mask & MASK_B) && cb + 1 <= t_b) {
          dp_next[ca][cb + 1] += ways;
        }
        // Assign to C
        if (mask & MASK_C) {
          // C's count increments implicitly: no change to (ca, cb)
          dp_next[ca][cb] += ways;
        }
      }
    }
    dp_prev.swap(dp_next);
  }

  return dp_prev; // size (t_a+1) x (t_b+1)
}

// Utility to set per-card probabilities in Pa, Pb, Pc
inline void setCardProbs(ProbHand &Pa, ProbHand &Pb, ProbHand &Pc,
                         const Card &c, double pa, double pb, double pc) {
  Pa.update(c, pa);
  Pb.update(c, pb);
  Pc.update(c, pc);
}

// Main API:
// Returns true on success, false if inputs are inconsistent (still fills what
// it can).
bool computeProbabilitiesDP(const Hand &Ha, const Hand &Hb, const Hand &Hc,
                            const Hand &Hab, const Hand &Hbc, const Hand &Hca,
                            const Hand &Habc, int Na, int Nb, int Nc,
                            ProbHand &Pa, ProbHand &Pb, ProbHand &Pc) {
  // Clear outputs
  Pa.clear();
  Pb.clear();
  Pc.clear();

  // Known singletons counts and capacities t_j = N_j - |Hj|
  const int Ka = (Ha.nbr_cards);
  const int Kb = (Hb.nbr_cards);
  const int Kc = (Hc.nbr_cards);

  int t_a = Na - Ka;
  int t_b = Nb - Kb;
  int t_c = Nc - Kc;

  if (t_a < 0 || t_b < 0 || t_c < 0) {
    // Inconsistent: known singletons exceed targets
    return false;
  }

  // Build unknown universe U and per-card allowed masks from ambiguous sets
  std::vector<Card> U;
  std::vector<int> allowedMask;
  buildUnknownUniverseAndMasks(Habc, Hab, Hbc, Hca, U, allowedMask);
  const int Usize = static_cast<int>(U.size());

  // Basic consistency: total unknown to be assigned must match
  if (t_a + t_b + t_c != Usize) {
    // Inconsistent counts; we proceed but results may be degenerate
    // You may prefer to return false here directly.
    // return false;
  }

  // First, set known cards: Pa=1 on Ha, etc.
  for (int su = 0; su < Card::N_SUITS; ++su) {
    for (int r = 0; r < Card::N_RANKS; ++r) {
      Card c(su, r);
      if (inHand(Ha, c)) {
        setCardProbs(Pa, Pb, Pc, c, 1.0, 0.0, 0.0);
      } else if (inHand(Hb, c)) {
        setCardProbs(Pa, Pb, Pc, c, 0.0, 1.0, 0.0);
      } else if (inHand(Hc, c)) {
        setCardProbs(Pa, Pb, Pc, c, 0.0, 0.0, 1.0);
      } else {
        // leave as zero for now; may be filled below if in U
      }
    }
  }

  // For each unknown card k in U, compute probabilities by DP counting
  // Note: For each k, we build masks excluding k, run DP once, and then read
  // counts for p in allowed(k).
  for (int idx = 0; idx < Usize; ++idx) {
    const Card &k = U[idx];
    const int mask_k = allowedMask[idx];

    // Build masks for the "other" cards
    std::vector<int> masksOther;
    masksOther.reserve(Usize - 1);
    for (int j = 0; j < Usize; ++j) {
      if (j == idx)
        continue;
      masksOther.push_back(allowedMask[j]);
    }
    const int M = static_cast<int>(masksOther.size()); // equals Usize - 1

    // Precompute DP table over A/B counts
    auto dpAB = countAssignmentsDP_AB(masksOther, t_a, t_b);

    // For each allowed player p, get completion count F_{k,p}
    long double F[3] = {0.0L, 0.0L, 0.0L};
    for (int p = 0; p < 3; ++p) {
      if (!(mask_k & (1 << p)))
        continue;

      const int ca_target = t_a - (p == A ? 1 : 0);
      const int cb_target = t_b - (p == B ? 1 : 0);
      const int cc_target = t_c - (p == C ? 1 : 0);

      // Targets must be within bounds
      if (ca_target < 0 || cb_target < 0 || cc_target < 0) {
        F[p] = 0.0L;
        continue;
      }
      // The implicit C count must match the remaining cards
      if (cc_target != (M - ca_target - cb_target)) {
        F[p] = 0.0L;
        continue;
      }
      if (ca_target > t_a || cb_target > t_b) {
        F[p] = 0.0L;
        continue;
      }
      F[p] = dpAB[ca_target][cb_target];
    }

    long double denom = 0.0L;
    if (mask_k & MASK_A)
      denom += F[A];
    if (mask_k & MASK_B)
      denom += F[B];
    if (mask_k & MASK_C)
      denom += F[C];

    double pa = 0.0, pb = 0.0, pc = 0.0;

    if (denom > 0.0L) {
      if (mask_k & MASK_A)
        pa = static_cast<double>(F[A] / denom);
      if (mask_k & MASK_B)
        pb = static_cast<double>(F[B] / denom);
      if (mask_k & MASK_C)
        pc = static_cast<double>(F[C] / denom);
    } else {
      // Inconsistent inputs for this card; fallback to uniform over allowed
      // players
      int allowedCount = ((mask_k & MASK_A) ? 1 : 0) +
                         ((mask_k & MASK_B) ? 1 : 0) +
                         ((mask_k & MASK_C) ? 1 : 0);
      double uniform = allowedCount > 0 ? (1.0 / allowedCount) : 0.0;
      if (mask_k & MASK_A)
        pa = uniform;
      if (mask_k & MASK_B)
        pb = uniform;
      if (mask_k & MASK_C)
        pc = uniform;
    }

    setCardProbs(Pa, Pb, Pc, k, pa, pb, pc);
  }

  // Optional: sanity check sums close to targets
  // You can enable/inspect these if desired.
  // double sumA = Pa.total();
  // double sumB = Pb.total();
  // double sumC = Pc.total();

  return true;
}

namespace {

// Clamp to [0,1]
inline double clamp01(double x) {
  if (x < 0.0)
    return 0.0;
  if (x > 1.0)
    return 1.0;
  return x;
}

// Product over cards in h_o that satisfy a suit/rank filter: ∏ (1 - Pp(card))
long double product_none_owned_in(
    const Hand &h_o,
    int suit_filter, // Suit code compatible with Card
    bool use_rank_filter,
    int rank_strict_greater_than, // Rank code; cards must have rank > this
    const ProbHand &Pp) {
  long double prod = 1.0L;
  for (int su = 0; su < Card::N_SUITS; ++su) {
    for (int r = 0; r < Card::N_RANKS; ++r) {
      Card c(su, r);
      if (!h_o.is_in(c))
        continue;
      if (su != suit_filter)
        continue;
      if (use_rank_filter && !(r > rank_strict_greater_than))
        continue;

      double p = Pp.getProb(c);
      p = clamp01(p);
      prod *= (1.0L - static_cast<long double>(p));
      if (prod <= 1e-18L)
        return 0.0L;
    }
  }
  if (prod < 0.0L)
    prod = 0.0L;
  if (prod > 1.0L)
    prod = 1.0L;
  return prod;
}

// Compute P(E_p): probability a single opponent p has a higher card than m_c
double prob_any_higher_for_player(const Hand &h_o, const Card &m_c,
                                  int s_led, // led suit
                                  int s_tr,  // trump suit
                                  const ProbHand &Pp) {
  const long double prod_led_any =
      product_none_owned_in(h_o, s_led, false, 0, Pp);
  const long double prod_led_high =
      product_none_owned_in(h_o, s_led, true, m_c.rnk, Pp);
  const long double prod_trump_any =
      product_none_owned_in(h_o, s_tr, false, 0, Pp);

  const long double P_L = 1.0L - prod_led_any;
  const long double P_H = 1.0L - prod_led_high;
  const long double P_T = 1.0L - prod_trump_any;

  // Unified conditional form
  const long double P_H_given_L = (P_L > 0.0L) ? (P_H / P_L) : 0.0L;
  const long double P_T_given_notL = (s_tr == s_led) ? 0.0L : P_T;

  const long double P_E = P_L * P_H_given_L + (1.0L - P_L) * P_T_given_notL;

  return clamp01(static_cast<double>(P_E));
}

} // namespace

// ord = 0: either opponent a or b has a higher card than m_c
double prob_higher_ord0(const Hand &h_o, const Card &m_c, int s_tr,
                        const ProbHand &Pa, const ProbHand &Pb) {
  const int s_led = m_c.su; // you lead

  const double Pa_higher =
      prob_any_higher_for_player(h_o, m_c, s_led, s_tr, Pa);
  const double Pb_higher =
      prob_any_higher_for_player(h_o, m_c, s_led, s_tr, Pb);

  const double none = (1.0 - Pa_higher) * (1.0 - Pb_higher);
  return clamp01(1.0 - none);
}

// ord = 1: b has led; only a can still beat your card
double prob_higher_ord1(const Hand &h_o, const Card &m_c,
                        int s_led, // suit led by b
                        int s_tr, const ProbHand &Pa) {
  return prob_any_higher_for_player(h_o, m_c, s_led, s_tr, Pa);
}

// ord = 2: b and c have played; only a remains
double prob_higher_ord2(const Hand &h_o, const Card &m_c,
                        int s_led, // suit led by b
                        int s_tr, const ProbHand &Pa) {
  return prob_any_higher_for_player(h_o, m_c, s_led, s_tr, Pa);
}

void SoundAgent::updateProbs() {
  computeProbabilitiesDP(Ha, Hb, Hc, Hab, Hbc, Hca, Habc, Ncards_a, Ncards_b,
                         Ncards_c, Pa, Pb, Pc);
}

Card SoundAgent::act(const State &state, const History &hist) {
  int op_team = (team_id + 1) % Hokm::N_TEAMS;
  LOG("--- " << name << " pl_id " << player_id << " team " << team_id << " ord "
             << (int)state.ord << " last_ord " << last_ord << " last_led "
             << Card::SU_STR[last_led] << " led " << Card::SU_STR[state.led]
             << " trump " << Card::SU_STR[state.trump] << " trick_id "
             << state.trick_id << "\nhand: " << hand.to_string());

  int b_id = (player_id + 3) % 4;
  int c_id = (player_id + 2) % 4;
  int a_id = (player_id + 1) % 4;

  bool critical =
      state.score[op_team] >= (Hokm::RND_WIN_SCORE - 2) ||
      state.score[op_team] > state.score[team_id] + Hokm::RND_WIN_SCORE / 2;
  LOG("critical: " << critical);

  Card c;
  switch (last_ord) {
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

  switch (state.ord) {
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
  switch (state.ord) {
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
  LOG("Pa.nbr: " << Pa.nbr() << ", Pb.nbr: " << Pb.nbr()
                 << ", Pc.nbr: " << Pc.nbr());
  LOG("Pa: " << Pa.to_string());
  LOG("Pb: " << Pb.to_string());
  LOG("Pc: " << Pc.to_string());
#ifdef DEBUG
  if (Pa.nbr() != Ncards_a || Pb.nbr() != Ncards_b || Pc.nbr() != Ncards_c) {
    LOG("Ncards_a: " << Ncards_a << " Ncards_b: " << Ncards_b
                     << " Ncards_c: " << Ncards_c);
    throw std::runtime_error("Nbr of cards inconsistencies!");
  }
#endif

  last_ord = state.ord;
  last_led = state.led;

  const Card &a_card = state.table[a_id];
  const Card &b_card = state.table[b_id];
  const Card &c_card = state.table[c_id];

  Card out = Card::NONE;

  switch (state.ord) {

  case 0: {
    LOG("case 0:");
    assert(a_card == Card::NONE && b_card == Card::NONE &&
           c_card == Card::NONE);

    Hand ps_ab = Habc.uni(Hca).uni(Hbc);
    Hand ps_play;
    Hand H_ab = Hab.uni(Ha).uni(Hb);
    Hand ps_hi = hand.lead_gt(H_ab, Card::NON_SU, state.trump);
    LOG("pssbl_hi_hand: " << ps_hi.to_string());
    if (ps_hi.nbr_cards == 0) {
      double prb_m = 0;
      for (Suit s = 0; s < Card::N_SUITS; s++)
        if (s != state.trump && hand.len[s]) {
          Card c_max = hand.top(s);
          int ls = hand.len[s] - 1;
          while (ls > 0) {
            ps_ab.rm_top(s, 1);
            double prb =
                1 - prob_higher_ord0(ps_ab, c_max, state.trump, Pa, Pb);
            if (prb > this->prob_floor) {
              if (prb > prb_m) {
                ps_play.clear();
                prb_m = prb;
                ps_play.add(hand.bottom(s));
              } else if (prb == prb_m)
                ps_play.add(hand.bottom(s));
              break;
            }
            ls--;
          }
        }
      LOG("No-hi-hand, pssbl. for the future: " << ps_play.to_string()
                                                << ", prb_m " << prb_m);
      Hand *h = (prb_m > 0) ? &ps_play : &hand;
      out = h->min_mil_trl(Card::NON_SU, state.trump);
      break;
    }
    ProbHand ps_prb_play;
    Card c;
    for (Suit s = 0; s < Card::N_SUITS; s++) {
      for (int i = 0; i < ps_hi.len[s]; i++) {
        c.set(s, ps_hi.cards[s][i]);
        double prb = 1 - prob_higher_ord0(ps_ab, c, state.trump, Pa, Pb);
        LOG("Card " << c.to_string() << ", prob: " << prb << " / "
                    << prob_floor);
        ps_prb_play.update(c, prb);
      }
    }
    ps_play = ps_prb_play.gt(this->prob_floor);
    LOG("Psbl. play: " << ps_play.to_string());
    if (ps_play.nbr_cards == 0) {
      double prb_m = 0;
      for (Suit s = 0; s < Card::N_SUITS; s++)
        if (s != state.trump && hand.len[s]) {
          Card c_max = hand.top(s);
          int ls = hand.len[s] - 1;
          while (ls > 0) {
            ps_ab.rm_top(s, 1);
            double prb =
                1 - prob_higher_ord0(ps_ab, c_max, state.trump, Pa, Pb);
            if (prb > this->prob_floor) {
              if (prb > prb_m) {
                ps_play.clear();
                prb_m = prb;
                ps_play.add(hand.bottom(s));
              } else if (prb == prb_m)
                ps_play.add(hand.bottom(s));
              break;
            }
            ls--;
          }
        }
      LOG("No-hi-prob, pssbl. for the future: " << ps_play.to_string()
                                                << ", prb_m " << prb_m);
      Hand *h = (prb_m > 0) ? &ps_play : &hand;
      out = h->min_mil_trl(Card::NON_SU, state.trump);
      break;
    }
    Hand cert_h = ps_prb_play.gte(1.0).discard(state.trump);
    Hand *h = (cert_h.nbr_cards) ? &cert_h : &ps_play;
    out = h->min_mil_trl(state.led, state.trump);
  } break;

  case 1: {
    LOG("case 1:");
    assert(a_card == Card::NONE && b_card != Card::NONE &&
           c_card == Card::NONE);

    Hand H_a = Ha;
    H_a.add(b_card);
    Hand ps_hi = hand.lead_gt(H_a, state.led, state.trump);
    LOG("pssbl_hi_hand: " << ps_hi.to_string());
    if (ps_hi.nbr_cards == 0) {
      out = hand.min_mil_trl(state.led, state.trump);
      break;
    }
    ProbHand ps_prb_play;
    Hand ps_a = Habc.uni(Hca).uni(Hab);
    Card c;
    for (Suit s = 0; s < Card::N_SUITS; s++) {
      for (int i = 0; i < ps_hi.len[s]; i++) {
        c.set(s, ps_hi.cards[s][i]);
        double prb = 1 - prob_higher_ord1(ps_a, c, state.led, state.trump, Pa);
        LOG("Card " << c.to_string() << ", prob: " << prb << " / "
                    << prob_floor);
        ps_prb_play.update(c, prb);
      }
    }
    Hand ps_play = ps_prb_play.gt(this->prob_floor);
    LOG("Psbl. play: " << ps_play.to_string());
    if (ps_play.nbr_cards == 0) {
      out = hand.min_mil_trl(state.led, state.trump);
      break;
    }
    Hand cert_h = ps_prb_play.gte(1.0).discard(state.trump);
    Hand *h = (cert_h.nbr_cards) ? &cert_h : &ps_play;
    out = h->min_mil_trl(state.led, state.trump);
  } break;

  case 2: {
    LOG("case 2:");
    assert(a_card == Card::NONE && b_card != Card::NONE &&
           c_card != Card::NONE);
    Hand ps_a = Habc.uni(Hca).uni(Hab);
    if (Card::cmp(c_card, b_card, state.led, state.trump) > 0) {
      double pr_comp_na =
          1 - prob_higher_ord2(ps_a, c_card, state.led, state.trump, Pa);
      LOG("comp c " << c_card.to_string()
                    << ", prob. to be better than ps_a: " << pr_comp_na << ", critical: " << critical);
      if (!critical && pr_comp_na > this->prob_floor) {
        LOG("OK with comp c.");
        out = hand.min_mil_trl(state.led, state.trump);
        break;
      }
    } else
      LOG("comp c " << c_card.to_string()
                    << " is less than op b card: " << b_card.to_string());
    Hand H_a = Ha;
    H_a.add(b_card);
    Hand ps_hi = hand.lead_gt(H_a, state.led, state.trump);
    LOG("pssbl_hi_hand: " << ps_hi.to_string());
    if (ps_hi.nbr_cards == 0) {
      out = hand.maxLed_maxTr_min_mil(state.led, state.trump);
      break;
    }
    ProbHand ps_prb_play;
    Card c;
    for (Suit s = 0; s < Card::N_SUITS; s++) {
      for (int i = 0; i < ps_hi.len[s]; i++) {
        c.set(s, ps_hi.cards[s][i]);
        double prb = 1 - prob_higher_ord2(ps_a, c, state.led, state.trump, Pa);
        LOG("Card " << c.to_string() << ", prob: " << prb << " / "
                    << prob_floor);
        ps_prb_play.update(c, prb);
      }
    }
    Hand ps_play = ps_prb_play.gt(this->prob_floor);
    LOG("Pssbl. play: " << ps_play.to_string());
    if (ps_play.nbr_cards == 0) {
      out = hand.maxLed_maxTr_min_mil(state.led, state.trump);
      break;
    }
    Hand cert_h = ps_prb_play.gte(1.0).discard(state.trump);
    if (cert_h.nbr_cards) {
      out = cert_h.min_mil_trl(state.led, state.trump);
    } else
      out = ps_play.min_mil_trl(state.led, state.trump);
    H_a.add(ps_a);
    int dfn = H_a.diff_between(out, c_card);
    LOG("diff " << c_card.to_string() << "-" << out.to_string() << ": " << dfn);
    if ((out.su != state.trump || c_card.su == state.trump) && dfn >= 0) {
      LOG("Companion's card is good enough.");
      out = hand.min_mil_trl(state.led, state.trump);
    }
  } break;

  case 3: {
    LOG("case 3:");
    Card bst_op = (Card::cmp(b_card, a_card, state.led, state.trump) > 0)
                      ? b_card
                      : a_card;
    if (Card::cmp(c_card, bst_op, state.led, state.trump) > 0) {
      LOG("Companion's card is good enough.");
      out = hand.min_mil_trl(state.led, state.trump);
      break;
    }
    Hand hi_hand = hand.gt_c_led(bst_op, state.led, state.trump);
    LOG("hi_hand: " << hi_hand.to_string());
    if (hi_hand.nbr_cards) {
      out = hi_hand.min_mil_trl(state.led, state.trump);
    } else
      out = hand.min_mil_trl(state.led, state.trump);
  } break;
  default:
    throw std::logic_error("Shouldn't be here!");
  }
  hand.remove(out);
  LOG("--- " << name << ", out card: " << out.to_string()
             << ", led: " << Card::SU_STR[state.led]
             << ", trump: " << Card::SU_STR[state.trump]);
  if (last_led == Card::NON_SU)
    last_led = out.su;
  return out;
}

SoundAgent::SoundAgent(double min_prob, double trump_prb_cap, double max_prob)
    : Agent(), mt_rnd_gen(std::mt19937(std::random_device()())),
      prob_floor(min_prob), trump_prob_cap(trump_prb_cap),
      prob_ceiling(max_prob) {
  name = "AI_" + std::to_string(player_id);
}

void SoundAgent::set_probs(double prob_floor, double trump_prob_cap,
                           double prob_ceiling) {
  //	if (prob_floor < 1 && prob_ceiling > prob_floor && prob_ceiling > 0
  //			&& trump_prob_cap > 0 && trump_prob_cap > prob_floor){
  this->prob_floor = prob_floor;
  this->prob_ceiling = prob_ceiling;
  this->trump_prob_cap = trump_prob_cap;
  //	}
}

void SoundAgent::reset() {

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