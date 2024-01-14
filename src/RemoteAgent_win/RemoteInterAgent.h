/*
 * RemoteAgent.h
 *
 *  Created on: Jun 1, 2023
 *      Author: mear
 */

#ifndef REMOTEAGENT_H_
#define REMOTEAGENT_H_


#include <winsock2.h>

#include "InteractiveAgent.h"

class RemoteInterAgent: public InteractiveAgent {

	uint16_t port = 23345;

	WSADATA wsa_data;
	SOCKET server_socket;
	struct sockaddr_in server_address;
	SOCKET client_socket;
	struct sockaddr_in client_address;

	bool client_connected {false};

public:
	RemoteInterAgent(int player_id, bool show_hand = true);
	~RemoteInterAgent();

	bool start_server();
	bool server_conn_acc();
	void stop_server();

	void output(const std::string &) override ;
	std::string input(const std::string &prompt) override ;

	void end_game() override;
};

#endif /* REMOTEAGENT_H_ */
