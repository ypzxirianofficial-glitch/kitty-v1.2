#pragma once
#include "../core/offsets.h"
#include "../kitty/kitty.h"

#include <windows.h>
#include <lualib.h>

namespace extLuaState {
    enum lsvalenum {
        lsval0 = 0,
        lsval1 = 1,
        lsval2 = 2,
        lsval3 = 3,
        lsval4 = 4,
    };

    struct lsvalinfo {
        uintptr_t lua_State;
        bool failed;
    };

    inline lsvalinfo get_lua_state(lsvalenum lsValue, uintptr_t encryptedState) {
        const uint32_t encryptedLow = kitty.read<uint32_t>(encryptedState);
        const uint32_t encryptedHigh = kitty.read<uint32_t>(encryptedState + sizeof(uint32_t));

        if (encryptedLow == 0 || encryptedHigh == 0)
            return { 0, true };

        const uint32_t baseLow = static_cast<uint32_t>(encryptedState);
        uint32_t low = 0, high = 0;

        switch (lsValue) {
        case lsval0: return { 0, true };
        case lsval1: low = baseLow - encryptedLow;  high = baseLow - encryptedHigh;  break;
        case lsval2: low = encryptedLow - baseLow;  high = encryptedHigh - baseLow;  break;
        case lsval3: low = baseLow ^ encryptedLow;  high = baseLow ^ encryptedHigh;  break;
        case lsval4: low = baseLow + encryptedLow;  high = baseLow + encryptedHigh;  break;
        default:     return { 0, true };
        }

        const uintptr_t decryptedState = (static_cast<uint64_t>(high) << 32) | low;
        return { decryptedState, false };
    }

    static constexpr uintptr_t OFF_STACKSIZE = 0x70;
    static constexpr int       LUAI_STACKSIZE = 45;
    static constexpr auto      LS_ENC = extLuaState::lsval3;

    static inline bool decrypt_u32_vm3(uintptr_t field_addr, uint32_t* out) {
        uint32_t enc = kitty.read<uint32_t>(field_addr);
        if (!enc) return false;
        *out = enc ^ (uint32_t)field_addr;
        return true;
    }

    static inline uintptr_t decrypt_ptr_vm3(uintptr_t field_addr) {
        uint32_t lo = kitty.read<uint32_t>(field_addr);
        uint32_t hi = kitty.read<uint32_t>(field_addr + 4);
        if (!lo || !hi) return 0;
        uint32_t base = (uint32_t)field_addr;
        return ((uintptr_t)(hi ^ base) << 32) | (lo ^ base);
    }

    static bool looks_like_lua_thread(uintptr_t p) {
        if (!p || p < 0x10000) return false;
        uint32_t stacksize = 0;
        if (!decrypt_u32_vm3(p + OFF_STACKSIZE, &stacksize)) return false;
        return stacksize == LUAI_STACKSIZE;
    }

    static uintptr_t decrypt_lua_state_at(uintptr_t field_addr) {
        auto ls = get_lua_state(LS_ENC, field_addr);
        if (ls.failed) return 0;
        if (!looks_like_lua_thread(ls.lua_State)) return 0;
        return ls.lua_State;
    }

    inline uintptr_t get_lua_thread_from_script(uintptr_t script_addr) {
        if (!script_addr) return 0;

        uint32_t tid = kitty.read<uint32_t>(script_addr + offsets::lua_thread::thread_ref_id);
        if (!tid) return 0;

        if (uintptr_t t = decrypt_lua_state_at(script_addr + offsets::lua_thread::thread))
            return t;

        uintptr_t ref = decrypt_ptr_vm3(script_addr + offsets::lua_thread::ref);
        if (!ref) return 0;

        if (uintptr_t t = decrypt_lua_state_at(ref + 0x10))
            return t;

        for (int off = 0; off < 0x40; off += 8) {
            if (off == 0x10) continue;
            if (uintptr_t t = decrypt_lua_state_at(ref + off))
                return t;
        }
        return 0;
    }
}