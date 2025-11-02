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

int SoundAgent::s_id = 0;

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

// -----------------------------------------------------------------------------
// Numeric helpers
// -----------------------------------------------------------------------------
inline long double clamp01l(long double x) {
  if (x < 0.0L)
    return 0.0L;
  if (x > 1.0L)
    return 1.0L;
  return x;
}

// C(k,n)/C(m,n) with 0 <= n <= k <= m; O(n) stable product; stays in [0,1].
long double comb_ratio_same_n_product(std::int64_t k, std::int64_t m,
                                      std::int64_t n) {
  if (n < 0 || k < 0 || m < 0)
    return 0.0L;
  if (n == 0)
    return 1.0L;
  if (k < n || m < n || k > m)
    return 0.0L;
  if (k == m)
    return 1.0L;

  long double prod = 1.0L;
  for (std::int64_t i = 0; i < n; ++i) {
    long double num = static_cast<long double>(k - i);
    long double den = static_cast<long double>(m - i);
    if (den == 0.0L)
      return 0.0L;
    prod *= (num / den);
    if (prod == 0.0L)
      break;
  }
  return clamp01l(prod);
}

// log(C(k,n)/C(m,n)) via lgamma; robust for large params (may be slower).
long double log_comb_ratio_same_n(std::int64_t k, std::int64_t m,
                                  std::int64_t n) {
  if (n < 0 || k < 0 || m < 0)
    return -std::numeric_limits<long double>::infinity();
  if (n == 0)
    return 0.0L;
  if (k < n || m < n || k > m)
    return -std::numeric_limits<long double>::infinity();
  if (k == m)
    return 0.0L;

  long double lk =
      lgammal((long double)k + 1.0L) - lgammal((long double)(k - n) + 1.0L);
  long double lm =
      lgammal((long double)m + 1.0L) - lgammal((long double)(m - n) + 1.0L);
  return lk - lm;
}

// Wrapper: uses product for typical n, log-domain for very large n.
long double comb_ratio_same_n(std::int64_t k, std::int64_t m, std::int64_t n) {
  if (n <= 1000) {
    return comb_ratio_same_n_product(k, m, n);
  } else {
    return clamp01l(expl(log_comb_ratio_same_n(k, m, n)));
  }
}

// log binomial C(n,k) via lgamma (for multinomial pieces; avoids overflow).
inline long double log_nCk_ll(long double n, long double k) {
  if (k < 0.0L || k > n)
    return -std::numeric_limits<long double>::infinity();
  if (k == 0.0L || k == n)
    return 0.0L;
  return lgammal(n + 1.0L) - lgammal(k + 1.0L) - lgammal(n - k + 1.0L);
}

// -----------------------------------------------------------------------------
// Pool counting relative to m_c, s_led, s_tr
// -----------------------------------------------------------------------------
struct PoolCounts {
  int M = 0;      // pool size
  int a_led = 0;  // led-suit cards
  int h_high = 0; // higher led-suit cards (> m_c) within a_led
  int t_tr = 0;   // trump cards (disjoint from led if s_tr != s_led)
  int o_oth = 0;  // others
};

PoolCounts compute_pool_counts(const Hand &h_o, const Card &m_c, int s_led,
                               int s_tr) {
  PoolCounts pc{};
  for (int su = 0; su < Card::N_SUITS; ++su) {
    for (int r = 0; r < Card::N_RANKS; ++r) {
      Card c{su, r};
      if (!h_o.is_in(c))
        continue;
      ++pc.M;
      bool is_led = (su == s_led);
      bool is_tr = (su == s_tr);
      if (is_led) {
        ++pc.a_led;
        if (r > m_c.rnk)
          ++pc.h_high;
      } else if (is_tr && s_tr != s_led) {
        ++pc.t_tr;
      } else {
        ++pc.o_oth;
      }
    }
  }
  return pc;
}

// -----------------------------------------------------------------------------
// Single-opponent exact probability (ord = 1 or 2): opponent beats m_c
// -----------------------------------------------------------------------------
double prob_higher_single_exact(const Hand &h_o, const Card &m_c, int s_led,
                                int s_tr, int n_a) {
  PoolCounts pc = compute_pool_counts(h_o, m_c, s_led, s_tr);
  const int M = pc.M;
  if (n_a <= 0 || M <= 0 || n_a > M)
    return 0.0;

  if (s_tr == s_led) {
    // P(E) = 1 - C(M - h, n) / C(M, n)
    long double p_noH = comb_ratio_same_n(M - pc.h_high, M, n_a);
    return (double)clamp01l(1.0L - p_noH);
  }

  // s_tr != s_led
  const int a = pc.a_led;
  const int h = pc.h_high;
  const int t = pc.t_tr;

  // P(H) = 1 - C(M - h, n) / C(M, n)
  long double P_H = 1.0L - comb_ratio_same_n(M - h, M, n_a);

  // P(¬L ∧ T) = [C(M - a, n) - C(M - a - t, n)] / C(M, n)
  long double p_notL = comb_ratio_same_n(M - a, M, n_a);
  long double p_notL_notT = comb_ratio_same_n(M - a - t, M, n_a);
  long double P_notL_T = clamp01l(p_notL - p_notL_notT);

  long double P = P_H + P_notL_T;
  return (double)clamp01l(P);
}

// -----------------------------------------------------------------------------
// ord = 0 helper: probability b fails given remaining pool (no replacement)
// Conditions for failing to beat when s_tr != s_led:
//  - no higher led-suit
//  - and [has at least one led-low OR (no led AND no trump)]
// -----------------------------------------------------------------------------
long double prob_b_fail_given_rem(int h_rem, int aL_rem, int t_rem, int o_rem,
                                  int n_b) {
  int Mr = h_rem + aL_rem + t_rem + o_rem;
  if (n_b < 0 || n_b > Mr)
    return 0.0L;
  if (n_b == 0)
    return 1.0L;

  // No higher led-suit
  long double p_noH = comb_ratio_same_n(Mr - h_rem, Mr, n_b);
  if (p_noH == 0.0L)
    return 0.0L;

  int M_noH = Mr - h_rem;
  // Within no-high pool: aL, t, o are available
  long double p_al_ge1 = 1.0L - comb_ratio_same_n(t_rem + o_rem, M_noH, n_b);
  long double p_all_o = comb_ratio_same_n(o_rem, M_noH, n_b);

  long double p_fail_noH = clamp01l(p_al_ge1 + p_all_o);
  return clamp01l(p_noH * p_fail_noH);
}

// -----------------------------------------------------------------------------
// Two-opponent exact probability (ord = 0): you lead; any opponent beats
// n_a: cards that opponent a draws from pool h_o
// n_b: cards that opponent b draws from the remaining pool
// -----------------------------------------------------------------------------
double prob_higher_ord0_exact(const Hand &h_o, const Card &m_c, int s_tr,
                              int n_a, int n_b) {
  const int s_led = m_c.su;
  PoolCounts pc = compute_pool_counts(h_o, m_c, s_led, s_tr);
  const int M = pc.M;
  if (n_a < 0 || n_b < 0 || n_a + n_b > M)
    return 0.0;

  if (n_a == 0 && n_b == 0)
    return 0.0;

  // s_tr == s_led: "beats" == "has a higher led-suit"
  if (s_tr == s_led) {
    long double pA_noH = comb_ratio_same_n(M - pc.h_high, M, n_a);
    long double pB_noH_givenA =
        comb_ratio_same_n((M - n_a) - pc.h_high, (M - n_a), n_b);
    long double P_any = 1.0L - clamp01l(pA_noH * pB_noH_givenA);
    return (double)clamp01l(P_any);
  }

  // s_tr != s_led
  const int h = pc.h_high;
  const int aL = pc.a_led - pc.h_high; // led-low
  const int t = pc.t_tr;
  const int o = pc.o_oth;

  // Sum over a's failing selections (no high, and [al>=1 or (al=0 & t=0)])
  long double log_den_A = log_nCk_ll((long double)M, (long double)n_a);
  long double P_both_fail = 0.0L;

  for (int al_a = 0; al_a <= std::min(aL, n_a); ++al_a) {
    int rem_after_al = n_a - al_a;
    int t_a_max = std::min(t, rem_after_al);
    for (int t_a = 0; t_a <= t_a_max; ++t_a) {
      int o_a = rem_after_al - t_a;
      if (o_a < 0 || o_a > o)
        continue;

      bool a_fails = (al_a >= 1) || (al_a == 0 && t_a == 0);
      if (!a_fails)
        continue; // al=0 and t>=1 -> would beat

      // Probability of this exact (al_a, t_a, o_a) for a (no restriction on
      // order)
      long double log_num = log_nCk_ll((long double)aL, (long double)al_a) +
                            log_nCk_ll((long double)t, (long double)t_a) +
                            log_nCk_ll((long double)o, (long double)o_a);
      long double pA_this = expl(log_num - log_den_A);
      if (pA_this == 0.0L)
        continue;

      // Remaining counts for b (h remains since a had no high)
      int h_rem = h;
      int aL_rem = aL - al_a;
      int t_rem = t - t_a;
      int o_rem = o - o_a;

      long double pB_fail =
          prob_b_fail_given_rem(h_rem, aL_rem, t_rem, o_rem, n_b);
      if (pB_fail == 0.0L)
        continue;

      P_both_fail += pA_this * pB_fail;
    }
  }

  long double P_any = 1.0L - clamp01l(P_both_fail);
  return (double)clamp01l(P_any);
}

// -----------------------------------------------------------------------------
// Convenience wrappers (ord 1 and ord 2 are identical structurally)
// -----------------------------------------------------------------------------
double prob_higher_ord1_exact(const Hand &h_o, const Card &m_c, int s_led,
                              int s_tr, int n_a) {
  return prob_higher_single_exact(h_o, m_c, s_led, s_tr, n_a);
}
double prob_higher_ord2_exact(const Hand &h_o, const Card &m_c, int s_led,
                              int s_tr, int n_a) {
  return prob_higher_single_exact(h_o, m_c, s_led, s_tr, n_a);
}

Suit SoundAgent::call_trump(const CardStack &first_5cards) {
  Hand hand = first_5cards.to_Hand();
  Hand cmpHand = hand.cmpl();

  LOG(name << ", trump call, first5:\n" << hand.to_su_string());

  double scr[Card::N_SUITS];
  //   float beta = 1.0f / 6;
  Card c;
  for (Suit su = 0; su < Card::N_SUITS; su++) {
    double sum_r = 0;
    for (int s = 0; s < Card::N_SUITS; s++)
      for (int i = 0; i < hand.len[s]; i++)
        sum_r += 1 - prob_higher_ord0_exact(cmpHand, c.set(s, hand.cards[s][i]),
                                            su, 13, 13);
    //   sum_r += hand.cards[s][i];
    scr[su] = sum_r;
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
  LOG("Nu_a: " << Nu_a << ", Nu_b: " << Nu_b << ", Nu_c: " << Nu_c);
  LOG("Ncards_a: " << Ncards_a << " Ncards_b: " << Ncards_b
                   << " Ncards_c: " << Ncards_c);
  //   updateProbs();
  //   LOG("Pa.nbr: " << Pa.nbr() << ", Pb.nbr: " << Pb.nbr()
  //                  << ", Pc.nbr: " << Pc.nbr());
  //   LOG("Pa: " << Pa.to_string());
  //   LOG("Pb: " << Pb.to_string());
  //   LOG("Pc: " << Pc.to_string());
  // #ifdef DEBUG
  //   if (Pa.nbr() != Ncards_a || Pb.nbr() != Ncards_b || Pc.nbr() !=
  //   Ncards_c)
  //   {
  //     throw std::runtime_error("Nbr of cards inconsistencies!");
  //   }
  // #endif

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
            double prb = 1 - prob_higher_ord0_exact(ps_ab, c_max, state.trump,
                                                    Nu_a, Nu_b);
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
        double prb =
            1 - prob_higher_ord0_exact(ps_ab, c, state.trump, Nu_a, Nu_b);
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
            double prb = 1 - prob_higher_ord0_exact(ps_ab, c_max, state.trump,
                                                    Nu_a, Nu_b);
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
    LOG("Cert hand: " << cert_h.to_string());
    Hand *h = (cert_h.nbr_cards) ? &cert_h : &ps_play;
    out = h->min_mil_trl(state.led, state.trump);
    if (out.su == state.trump && state.led != state.trump) {
      out = ps_prb_play.gt(this->prob_floor - 0.15)
                .min_mil_trl(state.led, state.trump);
    }
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
        double prb =
            1 - prob_higher_ord1_exact(ps_a, c, state.led, state.trump, Nu_a);
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
    LOG("Cert hand: " << cert_h.to_string());
    Hand *h = (cert_h.nbr_cards) ? &cert_h : &ps_play;
    out = h->min_mil_trl(state.led, state.trump);
    if (out.su == state.trump && state.led != state.trump) {
      out = ps_prb_play.gt(this->prob_floor - 0.15)
                .min_mil_trl(state.led, state.trump);
    }
  } break;

  case 2: {
    LOG("case 2:");
    assert(a_card == Card::NONE && b_card != Card::NONE &&
           c_card != Card::NONE);
    Hand ps_a = Habc.uni(Hca).uni(Hab);
    if (Card::cmp(c_card, b_card, state.led, state.trump) > 0) {
      double pr_comp_na = 1 - prob_higher_ord2_exact(ps_a, c_card, state.led,
                                                     state.trump, Nu_a);
      LOG("comp c " << c_card.to_string() << ", prob. to be better than ps_a: "
                    << pr_comp_na << ", critical: " << critical);
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
      if (Card::cmp(b_card, out, state.led, state.trump) > 0)
        out = hand.min_mil_trl(state.led, state.trump);
      break;
    }
    ProbHand ps_prb_play;
    Card c;
    for (Suit s = 0; s < Card::N_SUITS; s++) {
      for (int i = 0; i < ps_hi.len[s]; i++) {
        c.set(s, ps_hi.cards[s][i]);
        double prb =
            1 - prob_higher_ord2_exact(ps_a, c, state.led, state.trump, Nu_a);
        LOG("Card " << c.to_string() << ", prob: " << prb << " / "
                    << prob_floor);
        ps_prb_play.update(c, prb);
      }
    }
    Hand ps_play = ps_prb_play.gt(this->prob_floor);
    LOG("Pssbl. play: " << ps_play.to_string());
    if (ps_play.nbr_cards == 0) {
      out = hand.maxLed_maxTr_min_mil(state.led, state.trump);
      if (Card::cmp(b_card, out, state.led, state.trump) > 0)
        out = hand.min_mil_trl(state.led, state.trump);
      break;
    }
    Hand cert_h = ps_prb_play.gte(1.0).discard(state.trump);
    LOG("Cert hand: " << cert_h.to_string());
    if (cert_h.nbr_cards) {
      out = cert_h.min_mil_trl(state.led, state.trump);
    } else {
      out = ps_play.min_mil_trl(state.led, state.trump);
      if (out.su == state.trump && state.led != state.trump) {
        out = ps_prb_play.gt(this->prob_floor - 0.15)
                  .min_mil_trl(state.led, state.trump);
      }
    }
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
  name = "AI_S" + std::to_string(s_id++);
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

  last_ord = -1;
  last_led = Card::NON_SU;
}