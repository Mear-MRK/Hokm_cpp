#pragma once

#include "Card.h"

namespace Hokm {
	inline const int N_PLAYERS = 4;
	inline const int N_TEAMS = 2;
//	inline const int N_PL_TEAM = N_PLAYERS / N_TEAMS;
	inline const int N_DELT = Card::N_CARDS / N_PLAYERS;
	inline const int N_TRICKS = N_DELT;

	inline const int WIN_SCORE = 7;
	inline const int RND_WIN_SCORE = 7;
	inline const int SHUFFLE_CMPLX = 2;
}
