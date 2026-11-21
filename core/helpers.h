#pragma once
#include "../init_resource.h"
#include <windows.h>
#include <vector>
#include <random>

namespace helpers {
    struct EnumCtx {
        DWORD target_pid;
        std::vector<HWND>* out;
    };

    inline BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lp) {
        auto* ctx = reinterpret_cast<EnumCtx*>(lp);

        DWORD window_pid = 0;
        GetWindowThreadProcessId(hwnd, &window_pid);

        if (window_pid == ctx->target_pid && IsWindowVisible(hwnd)) {
            ctx->out->push_back(hwnd);
        }
        return TRUE;
    }

    inline std::vector<HWND> get_hwnd(DWORD pid) { // get hwnds for roblox
        std::vector<HWND> result;
        EnumCtx ctx{ pid, &result };
        EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&ctx));
        return result;
    }

    inline bool force_foreground(HWND hwnd) { // ported to c++ function from funny executor
        if (!hwnd || !IsWindow(hwnd)) return false;

        HWND fore = GetForegroundWindow();
        if (fore == hwnd) return true;

        DWORD fore_thread = fore ? GetWindowThreadProcessId(fore, nullptr) : 0;
        DWORD curr_thread = GetCurrentThreadId();

        if (fore_thread && fore_thread != curr_thread) {
            AttachThreadInput(curr_thread, fore_thread, TRUE);
            BringWindowToTop(hwnd);
            SetForegroundWindow(hwnd);
            AttachThreadInput(curr_thread, fore_thread, FALSE);
        }
        else {
            BringWindowToTop(hwnd);
            SetForegroundWindow(hwnd);
        }

        return GetForegroundWindow() == hwnd;
    }

    inline void press_escape() { // press escape to execute initialization script
        INPUT in[2]{};
        in[0].type = INPUT_KEYBOARD;
        in[0].ki.wScan = 0x01;
        in[0].ki.dwFlags = KEYEVENTF_SCANCODE;
        in[1] = in[0];
        in[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
        SendInput(2, in, sizeof(INPUT));
    }

    inline HMODULE getrsmodule() { // get resource module
        HMODULE Module;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)getrsmodule, &Module);
        return Module;
    }

    inline std::string init_parser() { // parse initialization script from resources
        HMODULE module = getrsmodule();
        if (module == NULL)
            return "warn('Failed to initialize init: (?)')";

        HRSRC res = FindResourceW(module, MAKEINTRESOURCE(LUAU), MAKEINTRESOURCE(INIT));
        if (res == NULL)
            return "warn('Failed to initialize init: (?)')";

        HGLOBAL data = LoadResource(module, res);
        if (data == NULL)
            return "warn('Failed to initialize init: (?)')";

        void* locked = LockResource(data);
        if (locked == NULL)
            return "warn('Failed to initialize init: (?)')";

        return std::string(static_cast<char*>(locked));
    }

    inline void clear_console() { // clears console output
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coordScreen = { 0, 0 };
        DWORD cCharsWritten;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD dwConSize;
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
        dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten);
        FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
        SetConsoleCursorPosition(hConsole, coordScreen);
    }
}