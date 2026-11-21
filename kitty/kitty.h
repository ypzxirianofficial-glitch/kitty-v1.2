#pragma once

#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

extern "C" {
    NTSTATUS kitty_ntreadvirtualmemory(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    NTSTATUS kitty_ntwritevirtualmemory(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    NTSTATUS kitty_ntprotectvirtualmemory(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);
    NTSTATUS kitty_ntopenprocess(PHANDLE, ACCESS_MASK, PVOID, PVOID);
    NTSTATUS kitty_ntclose(HANDLE);
}

class Kitty {
public:
    uintptr_t base_addr = 0;
    DWORD pid = 0;
    HANDLE handle = nullptr;
    std::string proc_name = {};
    DWORD last_error = 0;

    Kitty() = default;
    ~Kitty() { detach(); }
    Kitty(const Kitty&) = delete;
    Kitty& operator=(const Kitty&) = delete;

    std::vector<DWORD> getpids(const std::wstring_view procName) { // skidded from xeno lol
        std::vector<DWORD> processIDs;
        HANDLE snapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (snapShot == INVALID_HANDLE_VALUE)
            return processIDs;
        PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(snapShot, &entry)) {
            if (_wcsicmp(procName.data(), entry.szExeFile) == 0) {
                processIDs.push_back(entry.th32ProcessID);
            }
            while (Process32NextW(snapShot, &entry)) {
                if (_wcsicmp(procName.data(), entry.szExeFile) == 0) {
                    processIDs.push_back(entry.th32ProcessID);
                }
            }
        }
        CloseHandle(snapShot);
        return processIDs;
    }

    bool attach(const std::string& name) {
        return attach(to_wide(name));
    }
    bool attach(const char* name) {
        return attach(std::string(name));
    }

    bool attach(const std::wstring& process_name) {
        detach();
        pid = find_pid(process_name);
        if (!pid) { last_error = GetLastError(); return false; }
        proc_name = to_narrow(process_name);
        return open(pid, process_name);
    }
    bool attach(const wchar_t* name) {
        return attach(std::wstring(name));
    }

    template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    bool attach(T process_id) {
        detach();
        pid = static_cast<DWORD>(process_id);
        return open(pid, L"");
    }

    bool detach() {
        if (handle) {
            kitty_ntclose(handle);
        }
        handle = NULL; pid = 0; base_addr = 0;
        return handle == NULL;
    }

    bool attached() const { return handle != nullptr; }

    template <typename T>
    T read(uintptr_t addr, size_t max_len = 256) const {
        if constexpr (std::is_same_v<T, std::string>) {
            return read_string(addr, max_len);
        }
        else {
            static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
            T value{};
            read_raw(addr, &value, sizeof(T));
            return value;
        }
    }

    bool read_raw(uintptr_t addr, void* buf, size_t size) const {
        if (!handle) return false;

        SIZE_T got = 0;
        NTSTATUS st = kitty_ntreadvirtualmemory(
            handle, reinterpret_cast<PVOID>(addr), buf, size, &got);

        if (st >= 0 && got == size) return true;

        const_cast<Kitty*>(this)->last_error = static_cast<DWORD>(st);
        return false;
    }

    std::vector<uint8_t> read_bytes(uintptr_t addr, size_t size) const {
        std::vector<uint8_t> out(size);
        if (!read_raw(addr, out.data(), size)) out.clear();
        return out;
    }

    std::string read_string(uintptr_t addr, size_t max_len = 256) const {
        std::string out;
        char chunk[64];
        while (out.size() < max_len) {
            size_t to_page_end = 0x1000 - (addr & 0xFFF);
            size_t n = (std::min)({ sizeof(chunk), to_page_end, max_len - out.size() });
            if (!read_raw(addr, chunk, n)) break;
            for (size_t i = 0; i < n; ++i) {
                if (chunk[i] == '\0') return out;
                out.push_back(chunk[i]);
            }
            addr += n;
        }
        return out;
    }

    template <typename T>
    T read_chain(uintptr_t addr, std::initializer_list<uintptr_t> offsets) const {
        for (uintptr_t off : offsets) {
            addr = read<uintptr_t>(addr);
            if (!addr) return T{};
            addr += off;
        }
        return read<T>(addr);
    }

    template <typename T>
    bool write(uintptr_t addr, const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            return write_raw(addr, value.c_str(), value.size() + 1);
        }
        else {
            static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
            return write_raw(addr, &value, sizeof(T));
        }
    }

    bool write_raw(uintptr_t addr, const void* buf, size_t size) {
        if (!handle) return false;

        SIZE_T put = 0;
        NTSTATUS st = kitty_ntwritevirtualmemory(
            handle,
            reinterpret_cast<PVOID>(addr),
            const_cast<void*>(buf),
            size, &put);

        if (st >= 0 && put == size) return true;

        last_error = static_cast<DWORD>(st);
        return false;
    }

    bool write_protected(uintptr_t addr, const void* buf, size_t size) {
        if (!handle) return false;

        PVOID  base = reinterpret_cast<PVOID>(addr);
        SIZE_T sz = size;
        ULONG  old = 0;

        NTSTATUS st = kitty_ntprotectvirtualmemory(
            handle, &base, &sz, PAGE_EXECUTE_READWRITE, &old);
        if (st < 0) { last_error = static_cast<DWORD>(st); return false; }

        bool ok = write_raw(addr, buf, size);

        ULONG tmp = 0;
        PVOID  base2 = reinterpret_cast<PVOID>(addr);
        SIZE_T sz2 = size;
        kitty_ntprotectvirtualmemory(handle, &base2, &sz2, old, &tmp);
        return ok;
    }

    template <typename T>
    bool write_protected(uintptr_t addr, const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        return write_protected(addr, &value, sizeof(T));
    }

    uintptr_t allocate(size_t size, DWORD protect = PAGE_READWRITE) {
        if (!handle) return 0;
        return reinterpret_cast<uintptr_t>(
            VirtualAllocEx(handle, nullptr, size,
                MEM_COMMIT | MEM_RESERVE, protect));
    }

    bool free(uintptr_t addr) {
        if (!handle || !addr) return false;
        return VirtualFreeEx(handle, reinterpret_cast<LPVOID>(addr),
            0, MEM_RELEASE) != 0;
    }

    bool protect(uintptr_t addr, size_t size, DWORD new_protect, DWORD* old_protect = nullptr) {
        if (!handle) return false;
        DWORD old = 0;
        BOOL ok = VirtualProtectEx(handle, reinterpret_cast<LPVOID>(addr),
            size, new_protect, &old);
        if (old_protect) *old_protect = old;
        return ok != 0;
    }

    uintptr_t module_base(const std::wstring& module_name) const {
        if (!pid) return 0;
        HANDLE snap = CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        MODULEENTRY32W me{ sizeof(me) };
        uintptr_t result = 0;
        for (BOOL ok = Module32FirstW(snap, &me); ok; ok = Module32NextW(snap, &me)) {
            if (_wcsicmp(me.szModule, module_name.c_str()) == 0) {
                result = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                break;
            }
        }
        CloseHandle(snap);
        return result;
    }

private:
    static std::wstring to_wide(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(n > 0 ? n - 1 : 0, L'\0');
        if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
        return w;
    }
    static std::string to_narrow(const std::wstring& w) {
        if (w.empty()) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string s(n > 0 ? n - 1 : 0, '\0');
        if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
        return s;
    }

    static DWORD find_pid(const std::wstring& name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W pe{ sizeof(pe) };
        DWORD result = 0;
        for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
            if (_wcsicmp(pe.szExeFile, name.c_str()) == 0) {
                result = pe.th32ProcessID;
                break;
            }
        }
        CloseHandle(snap);
        return result;
    }

    bool open(DWORD process_id, const std::wstring& main_module) {
        ACCESS_MASK access =
            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
            PROCESS_QUERY_INFORMATION;

        CLIENT_ID cid{};
        cid.UniqueProcess = reinterpret_cast<HANDLE>(
            static_cast<uintptr_t>(process_id));
        cid.UniqueThread = nullptr;

        OBJECT_ATTRIBUTES oa{};
        oa.Length = sizeof(OBJECT_ATTRIBUTES);

        HANDLE h = nullptr;
        NTSTATUS st = kitty_ntopenprocess(&h, access, &oa, &cid);

        if (st >= 0 && h) {
            handle = h;
        }
        else {
            handle = OpenProcess(access, FALSE, process_id);
        }

        if (!handle) { pid = 0; return false; }

        if (!main_module.empty()) {
            base_addr = module_base(main_module);
        }
        else {
            HANDLE snap = CreateToolhelp32Snapshot(
                TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id);
            if (snap != INVALID_HANDLE_VALUE) {
                MODULEENTRY32W me{ sizeof(me) };
                if (Module32FirstW(snap, &me))
                    base_addr = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                CloseHandle(snap);
            }
        }
        return true;
    }
};

inline Kitty kitty;