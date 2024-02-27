#include "RemoteInterAgent.h"

#include <iostream>

// #include <thread>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

RemoteInterAgent::RemoteInterAgent(bool show_hand) : InteractiveAgent(show_hand)
{
    port = 23345 + player_id;
    client_connected = false;
    name = "RT_" + std::to_string(player_id);
    start_server();
    while (!server_conn_acc())
    {
    };
}

bool RemoteInterAgent::start_server()
{

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << name << ": ";
        perror("Socket creation failed");
        return false;
    }
    int opt = 1;
    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << name << ": ";
        perror("Setsockopt failed");
        return false;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    // Bind the socket to specified IP and port
    while (bind(server_socket, (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0)
    {
        if (port >= UINT16_MAX)
        {
            std::cerr << name << ": ";
            perror("Bind failed");
            close(server_socket);
            return false;
        }
        port++;
        server_address.sin_port = htons(port);
    }

    // Listen for incoming connections
    if (listen(server_socket, 0) < 0)
    {
        std::cerr << name << ": ";
        perror("Listen failed");
        close(server_socket);
        return false;
    }

    std::cout << name << ": Server listening on port "
              << port << std::endl;

    return true;
}

void RemoteInterAgent::stop_server()
{

    // Start a new thread for the connection accept loop
    //	std::thread acceptThread(ConnectionAcceptLoop, serverSocket);

    // Close the client socket
    close(client_socket);

    // Stop the connection accept loop
    close(server_socket);
    //	acceptThread.join();
}

void RemoteInterAgent::output(const std::string &out_str)
{
    std::string output_str = out_str + "[SEP]";
    const char *message = output_str.c_str();

    // Send message to client
    while (send(client_socket, message, strlen(message), 0) < 0)
    {
        std::cerr << name << ": ";
        perror("Send failed");
        close(client_socket);
        client_connected = false;
        while (!server_conn_acc())
        {
        };
    }
}

RemoteInterAgent::~RemoteInterAgent()
{
    stop_server();
}

bool RemoteInterAgent::server_conn_acc()
{
    if(client_connected)
        return true;
    
    std::cout << name << ": Server waiting for client connection..." << std::endl;

    int clientAddressSize = sizeof(client_address);

    // Accept incoming connection
    client_socket = accept(server_socket,
                           (struct sockaddr *)&client_address, (socklen_t*)&clientAddressSize);
    if (client_socket < 0)
    {
        std::cerr << name << ": ";
        perror("Accept failed");
        return false;
    }

    std::cout << name << ": Client connected"
              << std::endl;
    client_connected = true;
    return true;
}

std::string RemoteInterAgent::input(const std::string &prompt)
{

    output("/INP" + prompt);

    const int buff_size = 4 * 1024;
    char buffer[buff_size] = {0};

    // Read data from client
    int nbr_bytes_recv = recv(client_socket, buffer, buff_size, 0);
    while (nbr_bytes_recv < 0)
    {
        std::cerr << name << ": ";
        perror("Read failed");
        close(client_socket);
        client_connected = false;
        while (!server_conn_acc())
        {
        };
    }
    if (nbr_bytes_recv)
    {
        buffer[nbr_bytes_recv - 1] = 0;
        std::string inp_str = std::string(buffer);
        LOG(name << ": Input: " << inp_str);
        return inp_str;
    }
    return "";
}

void RemoteInterAgent::fin_game()
{
    output("/END");
}
