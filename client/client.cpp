#include "client.h"
#include "../kitty/kitty.h"
#include "../core/instance.h"
#include "../core/bytecode.h"
#include "../core/helpers.h"
#include "../server/server.h"
#include "../execution/execution.h"
#include "../debugger.h"
#include "../core/luaState.h"
#include <windows.h>
#include <thread>
#include <atomic>

static bool process_alive(DWORD pid) {
    if (!pid) return false;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    DWORD code = 0;
    bool ok = GetExitCodeProcess(h, &code) && code == STILL_ACTIVE;
    CloseHandle(h);
    return ok;
}

kitty_client::kitty_client(int pid) : pid(pid) {
    version = "1.2";
    username = "";
    worker = std::thread(&kitty_client::initialize_client, this);
}

kitty_client::~kitty_client() {
    stop();
    if (worker.joinable()) worker.join();
}

void kitty_client::stop() {
    stop_flag = true;
}

bool kitty_client::isProcessAlive() {
    return process_alive((DWORD)pid);
}

void kitty_client::initialize_client() {
    if (!kitty.attach(pid)) {
        alive = false;
        return;
    }
    attached = true;

    uintptr_t base = kitty.base_addr;
    if (!base) { alive = false; return; }

    std::string client_pid = std::to_string(pid);

    int port = 3110 + (pid % 1000);

    if (!server::start(port, client_pid)) {
        std::cerr << "Server failed to start on " << port << " port!\n";
    }

    std::cout << "Server started on http://127.0.0.1:" << port << " url!\n";

    std::string client_folder_name = client_pid;

    uintptr_t last_datamodel = 0;
    bool first_print = true;

    while (!stop_flag && isProcessAlive()) {
        uintptr_t fake = kitty.read<uintptr_t>(base + offsets::datamodel::pointer);
        uintptr_t real = fake ? kitty.read<uintptr_t>(fake + offsets::datamodel::real) : 0;

        if (real != last_datamodel) {
            if (real) {
                std::string dm_name = rbx::instance(real).name();
                std::cout << "[" << pid << "] Found DataModel (0x" << std::hex << real << ") (" << dm_name << ")\n";

                if (dm_name == "Ugc") {
                    if (!rbx::instance(real).find({ "CoreGui", client_folder_name })) {
                        auto hwnds = helpers::get_hwnd(pid);
                        HWND hwnd = hwnds.empty() ? nullptr : hwnds.front();

                        rbx::instance game = rbx::instance(real);

                        rbx::script_context sc = rbx::script_context(game).get_script_context();
                        if (sc.address) sc.set_bypass(true);

                        while (!kitty.read<bool>(game.address + offsets::datamodel::game_loaded)
                            && !stop_flag && isProcessAlive())
                            Sleep(50);

                        rbx::instance players;
                        do {
                            players = game.find_first_child("Players");
                            Sleep(50);
                        } while ((!players || players.get_children().empty())
                            && !stop_flag && isProcessAlive());

                        rbx::instance local_player = players.local_player();
                        if (local_player.address) username = local_player.name();

                        log_debug() << "Username found: '" << username << "'";

                        std::string luau_init = helpers::init_parser();
                        bytecode::repStr(luau_init, "%client_port%", std::to_string(port));
                        bytecode::repStr(luau_init, "%client_pid%", client_folder_name);
                        bytecode::repStr(luau_init, "%client_version%", globals::client_version);
                        bytecode::repStr(luau_init, "%client_name%", globals::client_name);

                        std::vector<uint8_t> bytes = bytecode::prepare(luau_init+"return setmetatable({}, {__index = function()return function() end;end})");

                        rbx::script plm = game.find({
                            "CoreGui","RobloxGui","Modules","PlayerList","PlayerListManager"
                            });

                        rbx::script event_conn = game.find({
                            "CoreGui","RobloxGui","Modules","Common","EventConnection"
                            });

                        /* module states
                        0 = unloaded module
                        1 = loading / yielded
                        2 = errored while loading
                        3 = loaded already and cached
                        */

                        if (event_conn.address)
                            kitty.write<int>(event_conn.address + offsets::script::modulescript::module_state, 0);

                        auto revert = event_conn.set_bytecode(bytes);
                        plm.spoof_to(event_conn);

                        HWND oldfg = GetForegroundWindow();
                        helpers::force_foreground(hwnd);
                        Sleep(50);
                        helpers::press_escape();
                        Sleep(50);
                        revert();
                        plm.spoof_to(plm);
                        helpers::press_escape();
                        if (oldfg && oldfg != hwnd) helpers::force_foreground(oldfg);

                        rbx::instance kitty_folder;
                        while (!kitty_folder.address && !stop_flag && isProcessAlive()) {
                            kitty_folder = game.find({ "CoreGui", client_folder_name });
                            Sleep(50);
                        }

                        {
                            std::lock_guard<std::mutex> lk(globals::state_mtx);
                            attached = true;
                        }
                        std::cout << "[" << pid << "] Kitty folder ready\n";
                    }
                    else {
                        rbx::instance game = rbx::instance(real);
                        rbx::instance players;
                        do {
                            players = game.find_first_child("Players");
                            Sleep(50);
                        } while ((!players || players.get_children().empty())
                            && !stop_flag && isProcessAlive());

                        rbx::instance local_player = players.local_player();
                        if (local_player.address) username = local_player.name();

                        log_debug() << "Username found: '" << username << "'";

                        std::lock_guard<std::mutex> lk(globals::state_mtx);
                        attached = true;
                    }
                }
                else if (dm_name == "LuaApp") {
                    rbx::instance game = rbx::instance(real);
                    rbx::script_context(game.address).get_script_context().set_bypass(false);

                    rbx::instance players;
                    do {
                        players = game.find_first_child("Players");
                        Sleep(50);
                    } while ((!players || players.get_children().empty())
                        && !stop_flag && isProcessAlive());

                    rbx::instance local_player = players.local_player();
                    if (local_player.address) username = local_player.name();

                    std::lock_guard<std::mutex> lk(globals::state_mtx);
                    attached = false;
                }
            }
            else if (!first_print) {
                std::cout << "[" << pid << "] DataModel lost\n";
            }
            last_datamodel = real;
            first_print = false;
        }

        Sleep(100);
    }

    kitty.detach();
    alive = false;
    attached = false;
}