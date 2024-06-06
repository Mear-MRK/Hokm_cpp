#pragma once

#include <netinet/in.h>

#include "InteractiveAgent.h"

class RemoteInterAgent: public InteractiveAgent {

	in_port_t port;

	int server_socket;
	struct sockaddr_in server_address;
	int client_socket;
	struct sockaddr_in client_address;

	bool client_connected {false};

public:
	RemoteInterAgent(bool show_hand = true);
	~RemoteInterAgent();

	bool start_server();
	bool server_conn_acc();
	void stop_server();

	void output(const std::string &) override ;
	std::string input(const std::string &prompt) override ;

	void fin_game() override;
};
