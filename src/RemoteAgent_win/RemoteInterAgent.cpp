/*
 * RemoteAgent.cpp
 *
 *  Created on: Jun 1, 2023
 *      Author: mear
 */

#include "RemoteInterAgent.h"

#include <iostream>
#include <thread>

#include "utils.h"

RemoteInterAgent::RemoteInterAgent(int player_id, bool show_hand) :
		InteractiveAgent(player_id, show_hand, false) {
	start_server();
	while(!server_conn_acc()){};
	output("/ALRGreetings player " + std::to_string(player_id) + "\nYou're part of team " + std::to_string(player_id % 2));
}


bool RemoteInterAgent::start_server() {

	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		std::cerr << "RemoteInterAgent " << player_id
				<< ": Failed to initialize Winsock" << std::endl;
		return false;
	}

	// Create socket
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		std::cerr << "RemoteInterAgent " << player_id << ": Socket creation failed: "
				<< WSAGetLastError() << std::endl;
		WSACleanup();
		return false;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);

	// Bind the socket to specified IP and port
	while (bind(server_socket, (struct sockaddr*) &server_address,
			sizeof(server_address)) == SOCKET_ERROR) {
		if (port >= UINT16_MAX) {
			std::cerr << "RemoteInterAgent " << player_id << ": Bind failed: "
					<< WSAGetLastError() << std::endl;
			closesocket(server_socket);
			WSACleanup();
			return false;
		}
		port++;
		server_address.sin_port = htons(port);
	}

	// Listen for incoming connections
	if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "RemoteInterAgent " << player_id << ": Listen failed: "
				<< WSAGetLastError() << std::endl;
		closesocket(server_socket);
		WSACleanup();
		return false;
	}

	std::cout << "RemoteInterAgent " << player_id << ": Server listening on port "
			<< port << std::endl;

	return true;

}

void RemoteInterAgent::stop_server() {

	// Start a new thread for the connection accept loop
	//	std::thread acceptThread(ConnectionAcceptLoop, serverSocket);


	// Close the client socket
	closesocket(client_socket);

	// Stop the connection accept loop
	closesocket(server_socket);
	//	acceptThread.join();

	// Cleanup Winsock
	WSACleanup();
}

void RemoteInterAgent::output(const std::string &out_str) {
	const char *message = out_str.c_str();


	// Send message to client
	while (send(client_socket, message, strlen(message), 0) == SOCKET_ERROR) {
		std::cerr << "RemoteInterAgent " << player_id << ": Send failed: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		client_connected = false;
		while(!server_conn_acc()){};
	}
}

RemoteInterAgent::~RemoteInterAgent() {
	stop_server();
}

bool RemoteInterAgent::server_conn_acc() {
	std::cout << "RemoteInterAgent " << player_id << ": Server waiting for client connection..." << std::endl;

		int clientAddressSize = sizeof(client_address);

		// Accept incoming connection
		client_socket = accept(server_socket,
				(struct sockaddr*) &client_address, &clientAddressSize);
		if (client_socket == INVALID_SOCKET) {
			std::cerr << "RemoteInterAgent " << player_id << ": Accept failed: "
					<< WSAGetLastError() << std::endl;
			return false;
		}

		std::cout << "RemoteInterAgent " << player_id << ": Client connected"
				<< std::endl;
		client_connected = true;
		return true;
}

std::string RemoteInterAgent::input(const std::string &prompt) {

	output(prompt + " " + "/INP");

	const int buff_size = 4 * 1024;
	char buffer[buff_size] = { 0 };

	// Read data from client
	int nbr_bytes_recv = recv(client_socket, buffer, buff_size, 0);
	while (nbr_bytes_recv == SOCKET_ERROR) {
		std::cerr << "RemoteInterAgent " << player_id << ": Read failed: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		client_connected = false;
		while(!server_conn_acc()){};
	}
	if (nbr_bytes_recv) {
		buffer[nbr_bytes_recv-1] = 0;
		std::string inp_str = std::string(buffer);
		LOG("RemoteInterAgent " << player_id << ": Input: " << inp_str);
		return inp_str;
	}
	return "";

}
