#pragma once

#include <string>

#include "GameConfig.h"
#include "Card.h"
#include "CardStack.h"

static inline std::string table_str_plpros(const Card table[], int player_id)
{
	int u = player_id;
	int b = (u + 3) % 4;
	int a = (u + 1) % 4;
	int c = (u + 2) % 4;

	std::string tbl_str = 
	"\t  " + table[c].to_string() 
	+ "\n\t" + table[b].to_string() + "  " + table[a].to_string()
	+ "\n\t  "+ table[u].to_string() + "\n";

	return tbl_str;
}

static inline std::string table_str(const Card table[])
{
	std::string tbl_str = "";
	for(int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		tbl_str += ((pl != 0) ? " " : "") + table[pl].to_string();

	return tbl_str;
}

static inline std::string table_str_with_names(const Card table[], const std::string name[]) {

  std::string out;
  for (size_t i = 0; i < Hokm::N_PLAYERS; ++i) {
    out += name[i];
    out += ':';
    out += table[i].to_string();
    out += ';';
  }
  return out;
}

static inline void table_clear(Card table[])
{
	for(int pl = 0; pl < Hokm::N_PLAYERS; pl++)
		table[pl] = Card::NONE;
}