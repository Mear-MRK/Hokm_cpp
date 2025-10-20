#pragma once

#include "InteractiveAgent.h"

// Forward-declare the shared server
class MultiClientServer;

class RemoteInterAgent : public InteractiveAgent {
public:
    RemoteInterAgent(bool show_hand = true);
    ~RemoteInterAgent() override = default;

    void output(const std::string&) override;
    std::string input(const std::string& prompt) override;
    void fin_game() override;
};
