#include "RemoteInterAgent.h"

#include <iostream>
#include <string>

#include "MultiClientServer.h"

RemoteInterAgent::RemoteInterAgent(bool show_hand)
    : InteractiveAgent(show_hand)
{
    name = "RT_" + std::to_string(player_id);

    // Ensure the shared server is up (single port for all players)
    MultiClientServer::instance().ensure_started(23345);

    // Reserve this slot so the next accepted connection is assigned here
    MultiClientServer::instance().reserve_slot(player_id);

    // Block until this player's client connects
    std::cout << name << ": waiting for client on shared server..." << std::endl;
    MultiClientServer::instance().wait_for_client(player_id);
    std::cout << name << ": client connected" << std::endl;
}

void RemoteInterAgent::output(const std::string& out_str)
{
    std::string msg = out_str + "[SEP]";
    MultiClientServer::instance().send_to(player_id, msg);
}

std::string RemoteInterAgent::input(const std::string& prompt)
{
    std::string request = "/INP" + prompt + "[SEP]";
    MultiClientServer::instance().send_to(player_id, request);
    std::string ans = MultiClientServer::instance().recv_from(player_id);
    if (!ans.empty() && ans.back() == '\r') ans.pop_back();
    return ans;
}

void RemoteInterAgent::fin_game()
{
    MultiClientServer::instance().send_to(player_id, std::string("/END[SEP]"));
}
