#pragma once
#include <Windows.h>
#include <string>
#include <mutex>
#include <vector>

namespace globals {
    struct state_mtx_t {
        std::mutex mtx;
        void lock() { mtx.lock(); }
        void unlock() { mtx.unlock(); }
    };
    inline std::mutex state_mtx;

    struct client_state {
        int pid = 0;
        bool attached = false;
        std::string version;
        std::string username;
        bool alive = true;
    };
}

class kitty_client {
public:
    explicit kitty_client(int pid);
    ~kitty_client();

    bool isProcessAlive();
    void stop();

    int pid;
    std::string version;
    std::string username;
    bool attached = false;
    bool alive = true;

private:
    void initialize_client();
    std::atomic<bool> stop_flag{ false };
    std::thread worker;
};

extern std::vector<std::shared_ptr<kitty_client>> g_clients;
extern std::mutex g_clients_mtx;

inline bool any_client_attached() {
    std::lock_guard<std::mutex> lk(g_clients_mtx);
    for (auto& c : g_clients) {
        if (c && c->alive && c->attached && c->isProcessAlive())
            return true;
    }
    return false;
}

inline bool client_attached(int pid) {
    std::lock_guard<std::mutex> lk(g_clients_mtx);
    for (auto& c : g_clients) {
        if (c && c->pid == pid)
            return c->alive && c->attached && c->isProcessAlive();
    }
    return false;
}

inline std::shared_ptr<kitty_client> find_client(int pid) {
    std::lock_guard<std::mutex> lk(g_clients_mtx);
    for (auto& c : g_clients)
        if (c && c->pid == pid) return c;
    return nullptr;
}