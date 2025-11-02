#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <netinet/in.h>

#include "GameConfig.h"
#include "ThreadSafeQueue.h"

class MultiClientServer {
public:
    static MultiClientServer& instance();

    // Starts the server if not already started
    void ensure_started(uint16_t port = 23345);

    // Graceful stop (optional to call)
    void stop();

    // Mark a slot as reserved so the next accepted client prefers this slot
    void reserve_slot(int player_id);

    // Clear a previously reserved slot
    void release_slot(int player_id);

    // Blocking: wait until the given player_id has a connected client
    void wait_for_client(int player_id);

    // Blocking receive next line (without trailing newline) from this player
    std::string recv_from(int player_id);

    // Send a string to this player (raw; caller adds [SEP] as needed)
    void send_to(int player_id, const std::string& msg);

    // Broadcast to all connected players
    void broadcast(const std::string& msg);

    bool is_connected(int player_id) const;

    bool any_connected() const;
    bool any_reserved() const;

private:
    MultiClientServer() = default;
    ~MultiClientServer();

    MultiClientServer(const MultiClientServer&) = delete;
    MultiClientServer& operator=(const MultiClientServer&) = delete;

    struct Session {
        int sock{-1};
        int player_id{-1};
        std::string name;
        std::atomic<bool> connected{false};
        std::thread reader_thread;
        ThreadSafeQueue<std::string> inbox;
        std::mutex send_mtx;
    };

    void accept_loop();
    void reader_loop(std::shared_ptr<Session> s);
    static bool send_all(int fd, const char* data, size_t len);
    static bool send_line(int fd, const std::string& line);
    static bool read_line(int fd, std::string& out); // reads until '\n'
    static void close_fd(int& fd);

    std::atomic<bool> started_{false};
    std::atomic<bool> shutting_down_{false};
    int listen_fd_{-1};
    uint16_t port_{23345};
    sockaddr_in addr_{};
    std::thread accept_thread_;

    mutable std::mutex m_;
    std::condition_variable cv_;
    std::array<std::shared_ptr<Session>, Hokm::N_PLAYERS> sessions_{};

    // Reservation flags: accept loop prefers assigning new connections to reserved slots
    std::array<bool, Hokm::N_PLAYERS> reserved_{};
};
