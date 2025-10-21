/*
 * InteractiveGame.cpp
 *
 *  Created on: May 27, 2023
 *      Author: mear
 */

#include "InteractiveGame.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "InteractiveAgent.h"
#include "RemoteInterAgent.h"
#include "SoundAgent.h"
#include "utils.h"

static Agent *newAgentFrom(char ag_typ, int, bool show_hand = true) {
  switch (ag_typ) {
  case 's':
    return new SoundAgent();
  case 'r':
    return new RemoteInterAgent(show_hand);
  case 'i':
    return new InteractiveAgent(show_hand);
  default:
    return nullptr;
  }
}

InteractiveGame::InteractiveGame(std::string ag_typs, bool show_hand,
                                 bool prompt)
    : prompt{prompt} {
  for (size_t pl = 0; pl < agent.size(); pl++)
    agent[pl] = newAgentFrom(ag_typs[pl], pl, show_hand);

  for (auto ag : agent)
    ag->init_game();

  round = std::unique_ptr<GameRound>(new GameRound(agent));
  round->show_info = true;
  if (prompt) {
    round->turn_sleep_ms = 0;
    round->rnd_sleep_ms = 0;
  }
}

InteractiveGame::~InteractiveGame() {
  for (auto ag : agent)
    delete ag;
}

int InteractiveGame::play(int win_score, int round_win_score) {
  int round_winner_team = -1;
  for (int r = 0; r <= 2 * win_score - 1; r++) {
    round->reset();
    std::string start_round_str = "/ALRWaiting for " +
                                  agent[round->opening_player]->get_name() +
                                  " to call the trump suit... ";
    broadcast_info(start_round_str);

    round->trump_call();

    broadcast_info("/INF" + agent[round->opening_player]->get_name() +
                   " called " + Card::SU_STR[round->state.trump] +
                   " as trump.");
    broadcast_info("/HED/TRM" + Card::SU_STR[round->state.trump]);

    round->deal_n_init();

    round_winner_team = round->play(round_win_score);

    team_scores[round_winner_team] += 1 + round->kot;

    std::string end_round_str =
        "/ALR=== Round " + std::to_string(r) +
        " winner team: " + std::to_string(round_winner_team) +
        "  (Kot: " + std::to_string(round->kot) + ") ===";
    LOG(end_round_str);
    broadcast_info(end_round_str);
    std::string game_scr_str =
        std::to_string(team_scores[0]) + ":" + std::to_string(team_scores[1]);
    broadcast_info("/HED/GSC" + game_scr_str);
    if (!prompt)
      std::this_thread::sleep_for(std::chrono::seconds(2));
    broadcast_info("/HED/RSC0:0");
    broadcast_info("/HED/TRM");
    broadcast_info("/ALR");

    if (team_scores[round_winner_team] >= win_score)
      break;
  }
  std::string fin_game_msg = "*** Team " + std::to_string(round_winner_team) +
                             " is the winner of the game. ***";
  LOG(fin_game_msg);
  broadcast_info("/ALR" + fin_game_msg);
  for (auto ag : agent)
    ag->fin_game();

  return round_winner_team;
}

void InteractiveGame::broadcast_info(std::string info_str, int exclude) {
  for (size_t pl = 0; pl < agent.size(); pl++)
    if ((int)pl != exclude)
      agent[pl]->info(info_str);
}
