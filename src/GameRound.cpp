#include "GameRound.h"

#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>

#include "Card.h"
#include "CardStack.h"
#include "Deck.h"
#include "GameConfig.h"
#include "Hand.h"
#include "table.h"
#include "utils.h"

GameRound::GameRound(std::array<Agent *, Hokm::N_PLAYERS> agent)
    : round_id(-1), hist(), deck(), agent(agent), team_scores({0}),
      mt_rnd_gen(std::mt19937(std::random_device()())), winner_team(-1),
      trump_team(-1), opening_player(-1), kot(0) {}

void GameRound::reset() {
  round_id++;
  state.reset();
  hist.reset();
  std::uniform_int_distribution<int> four_rnd(0, 3);
  if (winner_team == -1) {
    opening_player = four_rnd(mt_rnd_gen);
  } else if (winner_team != trump_team) {
    opening_player = (opening_player + 1) % Hokm::N_PLAYERS;
  }
  state.turn = opening_player;
  trump_team = opening_player % Hokm::N_TEAMS;
  if (collect.get_nbr_cards()) {
    LOG("Collected cards: " << collect.to_string());
    deck = Deck(collect);
    collect.clear();
  }
  deck.shuffle_deal(stack.data(), opening_player, Hokm::SHUFFLE_CMPLX);
  LOG("deck after shuffle: " << deck.to_cardStack().to_string());
  // stack[state.turn].shuffle();
  state.trump = -1;
  team_scores.fill(0);
  state.led = Card::NON_SU;
  winner_team = -1;
  for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
    this->agent[pl]->reset();
}

void GameRound::trump_call() {
  state.trump = this->agent[state.turn]->call_trump(stack[state.turn].top(5));
}

void GameRound::deal_n_init() {
  for (int pl = 0; pl < Hokm::N_PLAYERS; pl++) {
    hand[pl] = stack[pl].to_Hand();
    this->agent[pl]->init_round(hand[pl]);
  }
}

void GameRound::broadcast_info(std::string info_str, int exclude) {
  if (!show_info)
    return;
  for (int l = 0; l < Hokm::N_PLAYERS; l++)
    if (l != exclude)
      agent[l]->info(info_str);
}

inline int GameRound::trick() {
  state.reset();
  LOG("+++ Trick id " << (int)state.trick_id << " turn " << (int)state.turn
                      << " trump " << Card::SU_STR[state.trump] << " +++");
  broadcast_info("/INF--- Trick " + std::to_string(state.trick_id + 1) +
                 " ---");

  for (int ord = 0; ord < Hokm::N_PLAYERS; ord++) {
    int pl = (state.turn + ord) % Hokm::N_PLAYERS;
    state.ord = ord;

    auto ag = agent[pl];
    std::string nameId = ag->get_name(); // + " (id: " + std::to_string(ag->get_id()) + ")";

    broadcast_info("/ALRWaiting for " + nameId + " to play...");

    Card c = agent[pl]->act(state, hist);

    LOG(">>> " + agent[pl]->get_name() + " played " + c.to_string());
    broadcast_info("/ALR");
    broadcast_info("/INF" + nameId + " played " + c.to_string());
    if (show_info)
      agent[pl]->info("/HND" + agent[pl]->get_hand().to_string());

#ifdef DEBUG
    if (!hand[pl].card_is_valid_move(c, state.led)) {
      LOG("Game::trick: Card " + c.to_string() + " is not a valid act for " +
          agent[pl]->get_name() + " while led: " + Card::SU_STR[state.led] +
          ", hand: " + hand[pl].to_string());
      throw std::runtime_error("Illegal card played");
    }
#endif

    if (ord == 0)
      state.led = c.su;
    state.table[pl] = c;
    hand[pl].remove(c);

    broadcast_info("/TBL" + table_str(state.table));
    if (show_info)
      std::this_thread::sleep_for(std::chrono::milliseconds(turn_sleep_ms));
  }

  Card best_card = state.table[0];
  int best_pl = 0;
  for (int pl = 1; pl < Hokm::N_PLAYERS; pl++)
    if (Card::cmp(state.table[pl], best_card, state.led, state.trump) > 0) {
      best_card = state.table[pl];
      best_pl = pl;
    }

  for (int pl = 0; pl < Hokm::N_PLAYERS; pl++) {
    collect.append(state.table[pl]);
    hist.played_cards[pl].append(state.table[pl]);
    LOG(agent[pl]->get_name()
        << " played " << hist.played_cards[pl].at(-1).to_string());
  }

  LOG("^^^ Trick id " << (int)state.trick_id << " turn " << (int)state.turn
                      << " led " << Card::SU_STR[state.led] << " trump "
                      << Card::SU_STR[state.trump] << " ^^^");

  return best_pl;
}

int GameRound::play(int round_win_score) {
  broadcast_info("/INF=== Round " + std::to_string(round_id + 1) + " ===");
  kot = 0;
  for (int trick_id = 0; trick_id < Hokm::N_TRICKS; trick_id++) {
    state.trick_id = trick_id;
#ifdef DEBUG
    for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
      LOG(agent[pl]->get_name()
          << " hand " << agent[pl]->get_hand().to_string());
    LOG("=== Round " << round_id << ", Trick " << trick_id
                     << ", Turn: " << (int)state.turn
                     << ", Trump: " << Card::SU_STR[state.trump]);
#endif

    int trick_taker = trick();

    int trick_taker_team = trick_taker % Hokm::N_TEAMS;
    team_scores[trick_taker_team]++;
    state.turn = trick_taker;

    for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
      agent[pl]->trick_result(state, team_scores);

    bool round_finished = false;
    for (int team = 0; team < Hokm::N_TEAMS; team++)
      if (team_scores[team] >= round_win_score) {
        round_finished = true;
        winner_team = team;
        break;
      }

    {
      broadcast_info("/RSC" + std::to_string(team_scores[0]) + ":" +
                     std::to_string(team_scores[1]));
      broadcast_info("/INFLast table: " + table_str(state.table));
      if (show_info)
        std::this_thread::sleep_for(std::chrono::milliseconds(rnd_sleep_ms));
      table_clear(state.table);
      broadcast_info("/TBL" + table_str(state.table));
      if (round_finished)
        broadcast_info("/HND");
    }

    if (round_finished) {
      int oth_scr = 0;
      for (int team = 0; team < Hokm::N_TEAMS; team++)
        if (team != winner_team)
          oth_scr += team_scores[team];
      kot = (int)(oth_scr == 0) +
            (int)((oth_scr == 0) && (winner_team != trump_team));
      LOG("kot: " << kot << ", oth_scr: " << oth_scr << ", winner_team: "
                  << winner_team << ", trump_team: " << trump_team);
      break;
    }
  } // trick_id

  for (int pl = 0; pl < Hokm::N_PLAYERS; pl++)
    collect.append(hand[pl]);

  return winner_team;
}
