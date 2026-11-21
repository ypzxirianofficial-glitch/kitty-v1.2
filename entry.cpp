#include "kitty/kitty.h"
#include "core/instance.h"
#include "core/bytecode.h"
#include "core/helpers.h"
#include "client/client.h"
#include "server/server.h"
#include "execution/execution.h"
#include "debugger.h"

#include <iostream>
#include <conio.h>

struct clientInfo {
    const char* version;
    const char* username;
    int pid;
};

std::vector<std::shared_ptr<kitty_client>> g_clients;
std::mutex g_clients_mtx;

static std::atomic<bool> g_watcher_started{ false };

static void watcher_loop() {
    while (true) {
        std::vector<DWORD> pids = kitty.getpids(L"RobloxPlayerBeta.exe");
        std::vector<DWORD> truck = kitty.getpids(L"eurotrucks2.exe");
        pids.insert(pids.end(), truck.begin(), truck.end());
        {
            std::lock_guard<std::mutex> lk(g_clients_mtx);

            g_clients.erase(
                std::remove_if(g_clients.begin(), g_clients.end(),
                    [](const std::shared_ptr<kitty_client>& c) {
                        return !c->alive || !c->isProcessAlive();
                    }),
                g_clients.end());

            for (DWORD pid : pids) {
                bool exists = std::any_of(g_clients.begin(), g_clients.end(),
                    [&](const std::shared_ptr<kitty_client>& c) { return (DWORD)c->pid == pid; });
                if (!exists) {
                    auto c = std::make_shared<kitty_client>((int)pid);
                    g_clients.push_back(c);
                    log_info() << "Started client for PID " << pid;
                }
            }
        }
        Sleep(10);
    }
}

extern "C" {
    __declspec(dllexport) void attach(bool useConsole) {
        bool expected = false;
        if (!g_watcher_started.compare_exchange_strong(expected, true))
            return;
        if (useConsole) {
            FILE* conOut;
            AllocConsole();
            SetConsoleTitleA("Kitty");
            freopen_s(&conOut, "CONOUT$", "w", stdout);
            freopen_s(&conOut, "CONOUT$", "w", stderr);
        }
        std::thread(watcher_loop).detach();
    }

    __declspec(dllexport) clientInfo* allClients() {
        static std::vector<clientInfo> simpleClients;
        static std::vector<std::string> storage;

        std::lock_guard<std::mutex> lock(g_clients_mtx);

        simpleClients.clear();
        storage.clear();
        storage.reserve(g_clients.size() * 2);

        for (auto& c : g_clients) {
            if (!c->alive) continue;

            storage.push_back(c->version);
            storage.push_back(c->username);

            simpleClients.push_back({
                storage[storage.size() - 2].c_str(),
                storage[storage.size() - 1].c_str(),
                c->pid
                });
        }

        simpleClients.push_back({ nullptr, nullptr, 0 });
        return simpleClients.data();
    }

    __declspec(dllexport) bool executeScript(const char* source_cstr, int* pids, int num_pids) {
        if (!source_cstr) return false;
        std::string source(source_cstr);

        bool any = false;
        {
            std::lock_guard<std::mutex> lk(g_clients_mtx);
            for (auto& c : g_clients) {
                if (!c->alive || !c->attached || !c->isProcessAlive()) continue;
                bool match = (num_pids <= 0);
                for (int i = 0; !match && i < num_pids; ++i)
                    if (pids[i] == c->pid) match = true;
                if (match) { any = true; break; }
            }
        }
        if (!any) return false;

        execute(source);
        return true;
    }
}