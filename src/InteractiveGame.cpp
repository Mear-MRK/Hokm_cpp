/*
 * InteractiveGame.cpp
 *
 *  Created on: May 27, 2023
 *      Author: mear
 */

#include "InteractiveGame.h"

#include <chrono>
#include <string>
#include <thread>

#include "GameConfig.h"
#include "InteractiveAgent.h"
#include "MultiClientServer.h"
#include "RemoteInterAgent.h"
#include "SoundAgent.h"
#include "utils.h"
#include "table.h"  // for table_str(...)

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
  LOG("InteractiveGame(...) called.");
  for (size_t pl = 0; pl < Hokm::N_PLAYERS; pl++)
    agent[pl] = newAgentFrom(ag_typs[pl], pl, show_hand);

  for (auto ag : agent)
    ag->init_game();

  std::string info_str = "/INF";
  for (auto ag : agent)
    info_str += std::to_string(ag->get_id()) +
                ":" + ag->get_name() + ";";
  broadcast_info(info_str);

  round = std::unique_ptr<GameRound>(new GameRound(agent));
  round->show_info = true;
  if (prompt) {
    round->turn_sleep_ms = 0;
    round->rnd_sleep_ms = 0;
  }

  // NEW: register a resume handler to resend snapshot to a resuming player
  resume_handler_id = MultiClientServer::instance().add_resume_handler(
      [this](int pid) {
        try { this->sync_on_resume(pid); } catch (...) {}
      }
  );
}

InteractiveGame::~InteractiveGame() {
  LOG("~InteractiveGame() called.");
  // Unregister resume handler if still present
  if (resume_handler_id >= 0) {
    MultiClientServer::instance().remove_resume_handler(resume_handler_id);
    resume_handler_id = -1;
  }
  for (auto ag : agent)
    delete ag;
  MultiClientServer::instance().stop();
  Agent::reset_id();
}

int InteractiveGame::play(int win_score, int round_win_score) {
  int round_winner_team = -1;
  for (int r = 0; r <= 2 * win_score - 1; r++) {
    round->reset();
    auto o_ag = agent[round->opening_player];
    std::string o_nameId =
        o_ag->get_name(); //+ " (id: " + std::to_string(o_ag->get_id()) + ")";

    std::string start_round_str =
        "/ALRWaiting for " + o_nameId + " to call the trump suit... ";
    broadcast_info(start_round_str);

    round->trump_call();

    broadcast_info("/INF" + o_nameId + " called " +
                   Card::SU_STR[round->state.trump] + " as trump.");
    broadcast_info("/TRM" + Card::SU_STR[round->state.trump]);

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
    broadcast_info("/GSC" + game_scr_str);
    if (!prompt)
      std::this_thread::sleep_for(std::chrono::seconds(2));
    broadcast_info("/RSC0:0");
    broadcast_info("/TRM");
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

// NEW: per-seat state snapshot after resume
void InteractiveGame::sync_on_resume(int pid) {
  if (pid < 0 || pid >= (int)agent.size()) return;
  if (!round) return;

  
  // 4) Player's current hand (/HND)
  // Prefer agent's own view (may include last play), fallback to round's stored hand
  Hand ph = agent[pid]->get_hand();
  if (ph.nbr_cards == 0) {
    ph = round->get_player_hand(pid);
  }
  agent[pid]->info("/HND" + ph.to_string());

  // 1) Trump suit
  if (round->state.trump >= 0 && round->state.trump < Card::N_SUITS) {
    agent[pid]->info(std::string("/TRM") + Card::SU_STR[round->state.trump]);
  } else {
    // Clear trump if not set
    agent[pid]->info("/TRM");
    if (round->state.turn == pid){
      Hand h5 = round->stack[round->state.turn].top(5).to_Hand();
      agent[pid]->info("/ALRIt's your turn to call the trump.");
      agent[pid]->info("/HND" + h5.to_string());
    }
  }

  // 2) Current hand scores (/RSC)
  const auto& hsc = round->get_hand_scores();
  agent[pid]->info("/RSC" + std::to_string(hsc[0]) + ":" + std::to_string(hsc[1]));

  // 3) Current game scores (/GSC)
  agent[pid]->info("/GSC" + std::to_string(team_scores[0]) + ":" + std::to_string(team_scores[1]));


  // 5) Current table (/TBL)
  agent[pid]->info("/TBL" + table_str_with_names(round->state.table, round->name));
}
