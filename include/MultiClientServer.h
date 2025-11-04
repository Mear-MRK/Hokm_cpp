#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

    // Send a prompt to this player and remember it for replay upon reconnect
    void send_prompt(int player_id, const std::string& prompt_with_prefix /* e.g. "/INP..." without [SEP] */);

    // Broadcast to all connected players
    void broadcast(const std::string& msg);

    bool is_connected(int player_id) const;

    bool any_connected() const;
    bool any_reserved() const;

    // Resume handlers: game code can subscribe to be notified after a seat resumes
    int add_resume_handler(std::function<void(int /*player_id*/)> fn);
    void remove_resume_handler(int id);

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

        // Reconnect support
        std::string token;        // stable token for this seat
        std::string last_prompt;  // full framed prompt to replay: "/INP... [SEP]"
    };

    void accept_loop();
    void reader_loop(std::shared_ptr<Session> s);
    static bool send_all(int fd, const char* data, size_t len);
    static bool send_line(int fd, const std::string& line);
    static bool read_line(int fd, std::string& out); // reads until '\n'
    static bool read_line_timeout(int fd, std::string& out, int timeout_ms);
    static void close_fd(int& fd);
    static std::string generate_token();

    void fire_resume_handlers(int player_id);

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

    // token -> player_id
    std::unordered_map<std::string, int> token_to_pid_;

    // Resume handlers registry
    int next_handler_id_{1};
    std::vector<std::pair<int, std::function<void(int)>>> resume_handlers_;
};
