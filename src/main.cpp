/*
 * main.cpp
 *
 *  Created on: May 19, 2023
 *      Author: mear
 */

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <signal.h>


#include "InteractiveGame.h"
#include "LearningGame.h"
#include "utils.h"

#ifdef DEBUG
// void utils_test();
// void Card_test();
// void CardStack_test();
// void Hand_test();
// void Deck_test();
// void State_test();
// void History_test();
#endif

// #include "main_learning.cpp"

int main(int argc, char *argv[]) {
  struct sigaction sig_act = {};
  sig_act.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sig_act, NULL);
  srand(time(NULL));

  std::string agent_types = "srss";
  int game_win_score = Hokm::WIN_SCORE;
  int round_win_score = Hokm::RND_WIN_SCORE;
  switch (argc) {
  case 4:
    round_win_score = std::stoi(argv[3]);
  case 3:
    game_win_score = std::stoi(argv[2]);
  case 2:
    agent_types = std::string(argv[1]);
  }

  for (int i = 0; i < 10; i++) {
    InteractiveGame game(agent_types, true, true);
    game.play(game_win_score, round_win_score);
  }

  return 0;
}
