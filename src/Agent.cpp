#include "Agent.h"

#include "utils.h"

int Agent::next_id = 0;

Agent::Agent()
{
    player_id = next_id++;
    team_id = player_id % Hokm::N_TEAMS;
    name = "AG_" + std::to_string(player_id);
    hand = Hand::EMPTY;

    LOG("Agent(): name " << name << ", team " << team_id);
}

int Agent::get_id() const
{
    return player_id;
}

int Agent::get_team() const
{
    return team_id;
}

Hand Agent::get_hand() const
{
    return hand;
}

std::string Agent::get_name() const
{
    return name;
}
