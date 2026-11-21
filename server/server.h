#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "../execution/execution.h"
#include "../kitty/kitty.h"
#include "../core/instance.h"
#include "../core/bytecode.h"
#include "../globals.h"
#include "../client/client.h"

#include <httplib/httplib.h>
#include <luacode.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <bcrypt.h>
#include <wincrypt.h>
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace sha3_detail {
    static const uint64_t RC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL, 0x8000000080008000ULL,
        0x000000000000808bULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
        0x000000000000008aULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
        0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
        0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800aULL, 0x800000008000000aULL,
        0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
    };

    static const int ROTC[24] = {
        1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14, 27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
    };

    static const int PILN[24] = {
        10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4, 15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
    };

    static inline uint64_t rotl(uint64_t x, int n) { return (x << n) | (x >> (64 - n)); }

    static void keccakf(uint64_t st[25]) {
        int i, j, round;
        uint64_t t, bc[5];

        for (round = 0; round < 24; round++) {
            for (i = 0; i < 5; i++)
                bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

            for (i = 0; i < 5; i++) {
                t = bc[(i + 4) % 5] ^ rotl(bc[(i + 1) % 5], 1);
                for (j = 0; j < 25; j += 5)
                    st[j + i] ^= t;
            }

            t = st[1];
            for (i = 0; i < 24; i++) {
                j = PILN[i];
                bc[0] = st[j];
                st[j] = rotl(t, ROTC[i]);
                t = bc[0];
            }

            for (j = 0; j < 25; j += 5) {
                for (i = 0; i < 5; i++)
                    bc[i] = st[j + i];
                for (i = 0; i < 5; i++)
                    st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
            }

            st[0] ^= RC[round];
        }
    }

    static void sha3(const uint8_t* data, size_t len, uint8_t* out, int bits) {
        int rate = 200 - 2 * (bits / 8);
        uint64_t st[25] = { 0 };
        size_t i = 0;

        while (len - i >= (size_t)rate) {
            for (int j = 0; j < rate / 8; j++) {
                uint64_t word;
                std::memcpy(&word, data + i + j * 8, 8);
                st[j] ^= word;
            }
            keccakf(st);
            i += rate;
        }

        uint8_t tmp[200] = { 0 };
        std::memcpy(tmp, data + i, len - i);
        tmp[len - i] = 0x06;
        tmp[rate - 1] |= 0x80;

        for (int j = 0; j < rate / 8; j++) {
            uint64_t word;
            std::memcpy(&word, tmp + j * 8, 8);
            st[j] ^= word;
        }
        keccakf(st);
        std::memcpy(out, st, bits / 8);
    }
}

namespace sha2_detail {
    static inline uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    static const uint32_t K256[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    static void sha256_core(const uint8_t* data, size_t len, const uint32_t iv[8],
        uint8_t* out, int out_bytes) {
        uint32_t h[8];
        std::memcpy(h, iv, 32);
        uint64_t bitlen = (uint64_t)len * 8;

        size_t padded = ((len + 1 + 8) + 63) / 64 * 64;
        std::vector<uint8_t> buf(padded, 0);
        std::memcpy(buf.data(), data, len);
        buf[len] = 0x80;
        for (int i = 0; i < 8; i++)
            buf[padded - 1 - i] = (uint8_t)((bitlen >> (8 * i)) & 0xFF);

        for (size_t off = 0; off < padded; off += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; i++)
                w[i] = ((uint32_t)buf[off + i * 4] << 24) |
                ((uint32_t)buf[off + i * 4 + 1] << 16) |
                ((uint32_t)buf[off + i * 4 + 2] << 8) |
                (uint32_t)buf[off + i * 4 + 3];
            for (int i = 16; i < 64; i++) {
                uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
                uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            uint32_t a = h[0], b = h[1], c = h[2], d = h[3],
                e = h[4], f = h[5], g = h[6], hh = h[7];
            for (int i = 0; i < 64; i++) {
                uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
                uint32_t ch = (e & f) ^ (~e & g);
                uint32_t t1 = hh + S1 + ch + K256[i] + w[i];
                uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t t2 = S0 + maj;
                hh = g; g = f; f = e; e = d + t1;
                d = c; c = b; b = a; a = t1 + t2;
            }
            h[0] += a; h[1] += b; h[2] += c; h[3] += d;
            h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
        }

        uint8_t full[32];
        for (int i = 0; i < 8; i++) {
            full[i * 4] = (uint8_t)(h[i] >> 24);
            full[i * 4 + 1] = (uint8_t)(h[i] >> 16);
            full[i * 4 + 2] = (uint8_t)(h[i] >> 8);
            full[i * 4 + 3] = (uint8_t)(h[i]);
        }
        std::memcpy(out, full, out_bytes);
    }

    static const uint32_t IV224[8] = {
        0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
        0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
    };

    static std::vector<UCHAR> sha224(const std::string& data) {
        std::vector<UCHAR> out(28);
        sha256_core((const uint8_t*)data.data(), data.size(), IV224, out.data(), 28);
        return out;
    }

    static std::vector<UCHAR> hmac_sha224(const std::string& key, const std::string& data) {
        const size_t block = 64;
        std::vector<uint8_t> k(block, 0);
        if (key.size() > block) {
            auto kh = sha224(key);
            std::memcpy(k.data(), kh.data(), kh.size());
        }
        else {
            std::memcpy(k.data(), key.data(), key.size());
        }

        std::vector<uint8_t> ipad(block), opad(block);
        for (size_t i = 0; i < block; i++) { ipad[i] = k[i] ^ 0x36; opad[i] = k[i] ^ 0x5c; }

        std::string inner((char*)ipad.data(), block);
        inner.append(data);
        auto innerHash = sha224(inner);

        std::string outer((char*)opad.data(), block);
        outer.append((char*)innerHash.data(), innerHash.size());
        return sha224(outer);
    }
}

namespace server {
    static std::string hex_encode(const std::vector<UCHAR>& v) {
        static const char* hx = "0123456789abcdef";
        std::string out;
        out.reserve(v.size() * 2);
        for (UCHAR b : v) { out.push_back(hx[b >> 4]); out.push_back(hx[b & 0xF]); }
        return out;
    }

    static bool sha3_bits(const std::string& alg, int& bits) {
        if (alg == "SHA3-224") { bits = 224; return true; }
        if (alg == "SHA3-256") { bits = 256; return true; }
        if (alg == "SHA3-384") { bits = 384; return true; }
        if (alg == "SHA3-512") { bits = 512; return true; }
        return false;
    }

    static std::string hash_bcrypt(LPCWSTR alg_id, const std::string& data) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, alg_id, nullptr, 0))) return {};
        DWORD hash_len = 0, cb = 0;
        if (!BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
            (PUCHAR)&hash_len, sizeof(hash_len), &cb, 0))) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return {};
        }
        std::vector<UCHAR> digest(hash_len);
        NTSTATUS st = BCryptHash(hAlg, nullptr, 0,
            (PUCHAR)data.data(), (ULONG)data.size(),
            digest.data(), hash_len);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        if (!BCRYPT_SUCCESS(st)) return {};
        return hex_encode(digest);
    }

    static std::vector<UCHAR> hmac_bcrypt(LPCWSTR alg_id,
        const std::string& key, const std::string& data) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;
        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, alg_id, nullptr,
            BCRYPT_ALG_HANDLE_HMAC_FLAG))) return {};
        DWORD hash_len = 0, cb = 0;
        if (!BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
            (PUCHAR)&hash_len, sizeof(hash_len), &cb, 0))) {
            BCryptCloseAlgorithmProvider(hAlg, 0);
            return {};
        }
        std::vector<UCHAR> digest(hash_len);
        NTSTATUS st = BCryptHash(hAlg,
            (PUCHAR)key.data(), (ULONG)key.size(),
            (PUCHAR)data.data(), (ULONG)data.size(),
            digest.data(), hash_len);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        if (!BCRYPT_SUCCESS(st)) return {};
        return digest;
    }

    static std::vector<UCHAR> sha3_digest(const std::string& data, int bits) {
        std::vector<UCHAR> out(bits / 8);
        sha3_detail::sha3((const uint8_t*)data.data(), data.size(), out.data(), bits);
        return out;
    }

    static std::vector<UCHAR> hmac_sha3(const std::string& key, const std::string& data, int bits) {
        int block = 200 - 2 * (bits / 8);
        std::vector<uint8_t> k(block, 0);
        size_t copy_len = key.size() < (size_t)block ? key.size() : (size_t)block;
        std::memcpy(k.data(), key.data(), copy_len);
        std::vector<uint8_t> ipad(block), opad(block);
        for (int i = 0; i < block; i++) {
            ipad[i] = k[i] ^ 0x36;
            opad[i] = k[i] ^ 0x5c;
        }
        std::vector<uint8_t> inner(ipad.begin(), ipad.end());
        inner.insert(inner.end(), data.begin(), data.end());
        auto innerHash = sha3_digest(std::string(inner.begin(), inner.end()), bits);
        std::vector<uint8_t> outer(opad.begin(), opad.end());
        outer.insert(outer.end(), innerHash.begin(), innerHash.end());
        return sha3_digest(std::string(outer.begin(), outer.end()), bits);
    }

    inline std::string url_decode(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '%' && i + 2 < s.size()) {
                auto hex = [](char c) -> int {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                    return 0;
                    };
                out.push_back(static_cast<char>((hex(s[i + 1]) << 4) | hex(s[i + 2])));
                i += 2;
            }
            else if (s[i] == '+') {
                out.push_back(' ');
            }
            else {
                out.push_back(s[i]);
            }
        }
        return out;
    }

    inline std::map<std::string, std::string> parse_query(const std::string& q) {
        std::map<std::string, std::string> out;
        std::stringstream ss(q);
        std::string pair;
        while (std::getline(ss, pair, '&')) {
            auto eq = pair.find('=');
            if (eq == std::string::npos) {
                out[url_decode(pair)] = "";
            }
            else {
                out[url_decode(pair.substr(0, eq))] = url_decode(pair.substr(eq + 1));
            }
        }
        return out;
    }

    static std::string b64_decode(const std::string& in) {
        DWORD out_len = 0;
        if (!CryptStringToBinaryA(in.c_str(), (DWORD)in.size(), CRYPT_STRING_BASE64,
            nullptr, &out_len, nullptr, nullptr))
            return {};
        std::string out(out_len, '\0');
        if (!CryptStringToBinaryA(in.c_str(), (DWORD)in.size(), CRYPT_STRING_BASE64,
            reinterpret_cast<BYTE*>(out.data()), &out_len, nullptr, nullptr))
            return {};
        out.resize(out_len);
        return out;
    }

    static std::string b64_encode(const std::vector<UCHAR>& v) {
        DWORD len = 0;
        CryptBinaryToStringA(v.data(), (DWORD)v.size(),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &len);
        std::string s(len, '\0');
        CryptBinaryToStringA(v.data(), (DWORD)v.size(),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, s.data(), &len);
        s.resize(len);
        return s;
    }

    struct Context {
        std::string kitty_pid;
        uintptr_t base_addr = 0;
    };

    inline Context& ctx() {
        static Context c;
        return c;
    }

    inline const std::filesystem::path& get_workspace_path() {
        static std::filesystem::path workspace_path = []() {
            char exe_path[MAX_PATH];
            GetModuleFileNameA(NULL, exe_path, MAX_PATH);
            std::filesystem::path p(exe_path);
            p = p.parent_path() / "workspace";
            if (!std::filesystem::exists(p)) {
                std::filesystem::create_directories(p);
            }
            return p;
            }();
        return workspace_path;
    }

    static LPCWSTR pick_alg(const std::string& alg) {
        if (alg == "SHA1")       return BCRYPT_SHA1_ALGORITHM;
        if (alg == "SHA256")     return BCRYPT_SHA256_ALGORITHM;
        if (alg == "SHA384")     return BCRYPT_SHA384_ALGORITHM;
        if (alg == "SHA512")     return BCRYPT_SHA512_ALGORITHM;
        if (alg == "MD5")        return BCRYPT_MD5_ALGORITHM;
        return nullptr;
    }

    static std::string normalize_alg(std::string alg) {
        for (auto& c : alg) c = static_cast<char>(toupper((unsigned char)c));
        if (alg.rfind("SHA3", 0) == 0) {
            std::string rest = alg.substr(4);
            std::string digits;
            for (char c : rest) if (isdigit((unsigned char)c)) digits.push_back(c);
            if (digits == "224" || digits == "256" || digits == "384" || digits == "512")
                return "SHA3-" + digits;
        }
        return alg;
    }

    // hwid getter
    static std::string get_hwid()
    {
        HW_PROFILE_INFO hw_info{};
        std::string clean;

        if (GetCurrentHwProfile(&hw_info))
        {
            std::wstring w_guid(hw_info.szHwProfileGuid);
            std::string guid(w_guid.begin(), w_guid.end());
            for (char c : guid)
            {
                if (c != '{' && c != '}' && c != '-')
                    clean += c;
            }
        }

        if (clean.empty())
            clean = ("fallback");

        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE hash = nullptr;

        if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
            return clean;

        DWORD hash_length = 0;
        DWORD obj_length = 0;
        DWORD bytes_done = 0;

        BCryptGetProperty(algorithm,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&hash_length),sizeof(hash_length),&bytes_done,0);
        BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&obj_length),sizeof(obj_length),&bytes_done,0);

        std::vector<uint8_t> hash_obj(obj_length);
        std::vector<uint8_t> digest(hash_length);

        if (!BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &hash,
            hash_obj.data(), obj_length, nullptr, 0, 0)))
        {
            BCryptCloseAlgorithmProvider(algorithm, 0);
            return clean;
        }

        BCryptHashData(hash,reinterpret_cast<PUCHAR>(clean.data()),static_cast<ULONG>(clean.size()),0);

        char computer_name[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD name_len = sizeof(computer_name);
        if (GetComputerNameA(computer_name, &name_len))
            BCryptHashData(hash,reinterpret_cast<PUCHAR>(computer_name),name_len,0);

        DWORD volume_serial = 0;
        if (GetVolumeInformationA(("C:\\"), nullptr, 0,
            &volume_serial, nullptr, nullptr, nullptr, 0))
            BCryptHashData(hash,reinterpret_cast<PUCHAR>(&volume_serial),sizeof(volume_serial),0);

        BCryptFinishHash(hash, digest.data(), hash_length, 0);
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(algorithm, 0);

        std::string result(hash_length * 2, '\0');
        for (size_t i = 0; i < hash_length; i++)
            std::snprintf(&result[i * 2], 3, ("%02x"), digest[i]);

        return result;
    }

    class serve {
    public:
        static serve& instance() {
            static serve s;
            return s;
        }

        bool start(uint16_t port, const std::string& kitty_pid) {
            ctx().kitty_pid = kitty_pid;

            get_workspace_path();

            m_svr.Post("/ls", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);
                std::string module_name = q.count("c") ? q["c"] : "";
                if (module_name.empty()) {
                    res.status = 400;
                    res.set_content("no c parameter", "text/plain");
                    return;
                }

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                if (last_script.empty()) {
                    res.status = 204;
                    res.set_content("", "text/plain");
                    return;
                }

                std::string source = last_script;
                last_script.clear();

                std::string compiled = bytecode::compress(bytecode::__c(bytecode::sign(bytecode::compile("return function() " + source + " end"))));
                if (compiled.empty() || compiled[0] == '\0') {
                    res.status = 500;
                    res.set_content("compile failed", "text/plain");
                    return;
                }
                std::vector<uint8_t> bytes(compiled.begin(), compiled.end());

                uintptr_t fake = kitty.read<uintptr_t>(kitty.base_addr + offsets::datamodel::pointer);
                uintptr_t dm = fake ? kitty.read<uintptr_t>(fake + offsets::datamodel::real) : 0;
                if (!dm) {
                    res.status = 503;
                    res.set_content("no datamodel", "text/plain");
                    return;
                }

                rbx::instance game{ dm };
                rbx::instance container = game.find({ "CoreGui", ctx().kitty_pid });
                if (!container) {
                    res.status = 404;
                    res.set_content("no container", "text/plain");
                    return;
                }

                rbx::instance mod;
                for (int i = 0; i < 20; ++i) {
                    mod = container.find_first_child(module_name);
                    if (mod) break;
                    Sleep(1);
                }

                if (!mod) {
                    res.status = 404;
                    res.set_content("no module found with name: " + module_name, "text/plain");
                    return;
                }

                rbx::script s = rbx::script::from(mod);
                s.unlock_module();
                s.set_bytecode(bytes);

                res.status = 200;
                res.set_content("ok", "text/plain");
                });

            m_svr.Post("/loadstring", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);
                std::string module_name = q.count("c") ? q["c"] : "";
                if (module_name.empty()) {
                    res.status = 400;
                    res.set_content("no c parameter", "text/plain");
                    return;
                }

                std::string chunk_name = q.count("cn") ? q["cn"] : "";
                if (chunk_name.empty()) {
                    res.status = 400;
                    res.set_content("no cn parameter", "text/plain");
                    return;
                }

                std::string source = req.body;
                if (source.empty()) {
                    res.status = 400;
                    res.set_content("empty source", "text/plain");
                    return;
                }

                std::string compiled = bytecode::compress(bytecode::__c(bytecode::sign(bytecode::compile("return function(...)\n" + source + "\nend"))));
                if (compiled.empty() || compiled[0] == '\0') {
                    res.status = 500;
                    res.set_content("compile failed", "text/plain");
                    return;
                }
                std::vector<uint8_t> bytes(compiled.begin(), compiled.end());

                uintptr_t fake = kitty.read<uintptr_t>(kitty.base_addr + offsets::datamodel::pointer);
                uintptr_t dm = fake ? kitty.read<uintptr_t>(fake + offsets::datamodel::real) : 0;
                if (!dm) {
                    res.status = 503;
                    res.set_content("no datamodel", "text/plain");
                    return;
                }

                rbx::instance game{ dm };
                rbx::instance container = game.find({ "CoreGui", ctx().kitty_pid });
                if (!container) {
                    res.status = 404;
                    res.set_content("no container", "text/plain");
                    return;
                }

                rbx::instance mod;
                for (int i = 0; i < 20; ++i) {
                    mod = container.find_first_child(module_name);
                    if (mod) break;
                    Sleep(1);
                }

                if (!mod) {
                    res.status = 404;
                    res.set_content("no module found with name: " + module_name, "text/plain");
                    return;
                }

                rbx::script s = rbx::script::from(mod);
                s.unlock_module();
                s.set_bytecode(bytes);

                res.status = 200;
                res.set_content("ok", "text/plain");
                });

            m_svr.Post("/request", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                json opts;
                try {
                    opts = json::parse(req.body);
                }
                catch (const std::exception& e) {
                    res.status = 400;
                    res.set_content(std::string("invalid json: ") + e.what(), "text/plain");
                    return;
                }

                if (!opts.contains("Url") || !opts["Url"].is_string()) {
                    res.status = 400;
                    res.set_content("no Url", "text/plain");
                    return;
                }

                std::string url = opts["Url"].get<std::string>();
                std::string method = opts.value("Method", "GET");
                for (auto& c : method) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
                std::string body = opts.value("Body", "");

                httplib::Headers headers;
                if (opts.contains("Headers") && opts["Headers"].is_object()) {
                    for (auto& [k, v] : opts["Headers"].items()) {
                        if (v.is_string()) {
                            headers.emplace(k, v.get<std::string>());
                        }
                    }
                }

                headers.erase("User-Agent");
                headers.erase("user-agent");
                headers.emplace("User-Agent", globals::client_name + "/v" + globals::client_version);

                std::string hwid = get_hwid();

                auto has_header = [&](const std::string& key) -> bool {
                    for (auto& h : headers) {
                        if (_stricmp(h.first.c_str(), key.c_str()) == 0) return true;
                    }
                    return false;
                    };

                if (!has_header("Exploit-Identifier")) headers.emplace("Exploit-Identifier", "Kitty");
                if (!has_header("Exploit-Guid")) headers.emplace("Exploit-Guid", hwid);
                if (!has_header("Kitty-Fingerprint")) headers.emplace("Kitty-Fingerprint", hwid);
                if (!has_header("Kitty-User-Identifier")) headers.emplace("Kitty-User-Identifier", hwid);
                if (!has_header("Accept")) headers.emplace("Accept", "*/*");

                std::string host, path = "/";
                {
                    std::string u = url;
                    auto scheme_pos = u.find("://");
                    if (scheme_pos != std::string::npos) u = u.substr(scheme_pos + 3);
                    auto path_pos = u.find('/');
                    if (path_pos != std::string::npos) {
                        host = u.substr(0, path_pos);
                        path = u.substr(path_pos);
                    }
                    else {
                        host = u;
                    }
                }

                if (host.empty()) {
                    res.status = 400;
                    res.set_content("invalid url", "text/plain");
                    return;
                }

                httplib::Client cli(host);
                cli.set_connection_timeout(10, 0);
                cli.set_read_timeout(30, 0);
                cli.set_follow_location(true);

                httplib::Result result(nullptr, httplib::Error::Unknown);

                std::string content_type = "text/plain";
                if (headers.count("Content-Type")) content_type = headers.find("Content-Type")->second;
                else if (headers.count("content-type")) content_type = headers.find("content-type")->second;

                if (method == "GET") result = cli.Get(path, headers);
                else if (method == "POST") result = cli.Post(path, headers, body, content_type);
                else if (method == "PUT") result = cli.Put(path, headers, body, content_type);
                else if (method == "DELETE") result = cli.Delete(path, headers, body, content_type);
                else if (method == "PATCH") result = cli.Patch(path, headers, body, content_type);
                else {
                    res.status = 400;
                    res.set_content("unsupported method: " + method, "text/plain");
                    return;
                }

                json response;
                response["Success"] = false;
                response["StatusCode"] = 0;
                response["StatusMessage"] = "";
                response["Body"] = "";
                response["Headers"] = json::object();

                if (!result) {
                    response["StatusMessage"] = httplib::to_string(result.error());
                    response["StatusCode"] = 599;
                }
                else {
                    response["StatusCode"] = result->status;
                    response["StatusMessage"] = result->reason;
                    response["Body"] = result->body;
                    response["Success"] = result->status >= 200 && result->status < 300;

                    for (auto& [k, v] : result->headers) {
                        response["Headers"][k] = v;
                    }
                }

                res.status = 200;
                res.set_content(response.dump(), "application/json");
                });

            m_svr.Post("/setfpscap", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);
                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                if (!q.count("n")) {
                    res.status = 400;
                    res.set_content("no n parameter", "text/plain");
                    return;
                }

                int fps = 0;
                try {
                    fps = std::stoi(q["n"]);
                }
                catch (...) {
                    res.status = 400;
                    res.set_content("invalid n parameter", "text/plain");
                    return;
                }

                if (!kitty.write<int>(kitty.base_addr + offsets::task_scheduler::target_fps, fps)) {
                    res.status = 404;
                    res.set_content("failed to set fps", "text/plain");
                    return;
                }

                res.status = 200;
                res.set_content("ok", "text/plain");
                });

            m_svr.Post("/getfpscap", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                int fps = kitty.read<int>(kitty.base_addr + offsets::task_scheduler::target_fps);

                res.status = 200;
                res.set_content(std::to_string(fps), "text/plain");
                });

            m_svr.Post("/setclipboard", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                if (!q.count("t")) {
                    res.status = 400;
                    res.set_content("no t parameter", "text/plain");
                    return;
                }

                std::string text = q["t"];

                int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
                if (wlen <= 0) {
                    res.status = 500;
                    res.set_content("utf8 conversion failed", "text/plain");
                    return;
                }

                if (!OpenClipboard(nullptr)) {
                    res.status = 503;
                    res.set_content("cannot open clipboard", "text/plain");
                    return;
                }

                EmptyClipboard();

                HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
                if (!hmem) {
                    CloseClipboard();
                    res.status = 500;
                    res.set_content("global alloc failed", "text/plain");
                    return;
                }

                wchar_t* dst = static_cast<wchar_t*>(GlobalLock(hmem));
                MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, dst, wlen);
                GlobalUnlock(hmem);

                if (!SetClipboardData(CF_UNICODETEXT, hmem)) {
                    GlobalFree(hmem);
                    CloseClipboard();
                    res.status = 500;
                    res.set_content("set clipboard data failed", "text/plain");
                    return;
                }

                CloseClipboard();

                res.status = 200;
                res.set_content("ok", "text/plain");
                });

            m_svr.Post("/getclipboard", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                if (!OpenClipboard(nullptr)) {
                    res.status = 503;
                    res.set_content("", "text/plain");
                    return;
                }

                std::string out;
                HANDLE hdata = GetClipboardData(CF_UNICODETEXT);
                if (hdata) {
                    wchar_t* src = static_cast<wchar_t*>(GlobalLock(hdata));
                    if (src) {
                        int ulen = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
                        if (ulen > 0) {
                            out.resize(ulen - 1);
                            WideCharToMultiByte(CP_UTF8, 0, src, -1, out.data(), ulen, nullptr, nullptr);
                        }
                        GlobalUnlock(hdata);
                    }
                }

                CloseClipboard();

                res.status = 200;
                res.set_content(out, "text/plain");
                });

            m_svr.Post("/hash", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                if (!q.count("alg")) {
                    res.status = 400;
                    res.set_content("fail", "text/plain");
                    return;
                }

                std::string alg = normalize_alg(q["alg"]);
                std::string data = b64_decode(req.body);

                int bits = 0;
                if (sha3_bits(alg, bits)) {
                    auto digest = sha3_digest(data, bits);
                    res.status = 200;
                    res.set_content(hex_encode(digest), "text/plain");
                    return;
                }

                if (alg == "SHA224" || alg == "SHA-224") {
                    auto digest = sha2_detail::sha224(data);
                    res.status = 200;
                    res.set_content(hex_encode(digest), "text/plain");
                    return;
                }

                LPCWSTR alg_id = pick_alg(alg);
                if (!alg_id) {
                    res.status = 400;
                    res.set_content("fail", "text/plain");
                    return;
                }

                std::string out = hash_bcrypt(alg_id, data);
                if (out.empty()) {
                    res.status = 500;
                    res.set_content("fail", "text/plain");
                    return;
                }

                res.status = 200;
                res.set_content(out, "text/plain");
                });

            m_svr.Post("/hmac", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                if (!q.count("alg")) {
                    res.status = 400;
                    res.set_content("fail", "text/plain");
                    return;
                }

                std::string alg = normalize_alg(q["alg"]);

                auto nl = req.body.find('\n');
                if (nl == std::string::npos) {
                    res.status = 400;
                    res.set_content("fail", "text/plain");
                    return;
                }
                std::string key = b64_decode(req.body.substr(0, nl));
                std::string data = b64_decode(req.body.substr(nl + 1));

                int bits = 0;
                if (sha3_bits(alg, bits)) {
                    auto digest = hmac_sha3(key, data, bits);
                    res.status = 200;
                    res.set_content(b64_encode(digest), "text/plain");
                    return;
                }

                if (alg == "SHA224" || alg == "SHA-224") {
                    auto digest = sha2_detail::hmac_sha224(key, data);
                    res.status = 200;
                    res.set_content(b64_encode(digest), "text/plain");
                    return;
                }

                LPCWSTR alg_id = pick_alg(alg);
                if (!alg_id) {
                    res.status = 400;
                    res.set_content("fail", "text/plain");
                    return;
                }

                auto digest = hmac_bcrypt(alg_id, key, data);
                if (digest.empty()) {
                    res.status = 500;
                    res.set_content("fail", "text/plain");
                    return;
                }

                res.status = 200;
                res.set_content(b64_encode(digest), "text/plain");
                });

            m_svr.Post("/getscriptbytecode", [](const httplib::Request& req, httplib::Response& res) {
                std::string query;
                auto qpos = req.target.find('?');
                if (qpos != std::string::npos) query = req.target.substr(qpos + 1);

                if (!any_client_attached()) {
                    res.status = 400;
                    res.set_content("attach first", "text/plain");
                    return;
                }

                auto q = parse_query(query);

                std::string pid = q.count("p") ? q["p"] : "";
                if (pid.empty()) {
                    res.status = 400;
                    res.set_content("no p parameter", "text/plain");
                    return;
                }

                if (!(pid == std::to_string(kitty.pid))) {
                    res.status = 404;
                    res.set_content("pid mismatch", "text/plain");
                    return;
                }

                std::string holder_name = q.count("n") ? q["n"] : "";
                if (holder_name.empty()) {
                    res.status = 400;
                    res.set_content("no n parameter", "text/plain");
                    return;
                }

                uintptr_t fake = kitty.read<uintptr_t>(kitty.base_addr + offsets::datamodel::pointer);
                uintptr_t dm = fake ? kitty.read<uintptr_t>(fake + offsets::datamodel::real) : 0;
                if (!dm) {
                    res.status = 503;
                    res.set_content("no datamodel", "text/plain");
                    return;
                }

                rbx::instance game{ dm };
                rbx::instance container = game.find({ "CoreGui", ctx().kitty_pid });
                if (!container) {
                    res.status = 404;
                    res.set_content("no container", "text/plain");
                    return;
                }

                rbx::instance holder = container.find_first_child(holder_name);
                if (!holder) {
                    res.status = 404;
                    res.set_content("no holder", "text/plain");
                    return;
                }

                rbx::script script = holder.value();
                if (!script) {
                    res.status = 404;
                    res.set_content("no holder value found", "text/plain");
                    return;
                }

                std::vector<uint8_t> bytecode_bytes = script.get_authentic_bytecode();
                if (!bytecode_bytes.size()) {
                    res.status = 404;
                    res.set_content("no bytecode bytes found", "text/plain");
                    return;
                }

                res.status = 200;
                res.set_content(b64_encode(bytecode_bytes), "text/plain");
                });

            m_svr.set_read_timeout(0.5, 0);
            m_svr.set_write_timeout(0.5, 0);
            m_svr.set_keep_alive_max_count(100);
            m_svr.set_keep_alive_timeout(0.5);

            m_running.store(true);
            m_thread = std::thread([this, port] {
                if (!m_svr.listen("127.0.0.1", port)) {
                    m_running.store(false);
                }
                });
            return true;
        }

        void stop() {
            if (!m_running.exchange(false)) return;
            m_svr.stop();
            if (m_thread.joinable()) m_thread.join();
        }

        uint16_t port() const { return m_port; }

    private:
        serve() = default;
        ~serve() { stop(); }
        serve(const serve&) = delete;
        serve& operator=(const serve&) = delete;

        httplib::Server m_svr;
        std::thread m_thread;
        std::atomic<bool> m_running{ false };
        uint16_t m_port = 0;
    };

    inline bool start(uint16_t port, const std::string& kitty_pid) {
        return serve::instance().start(port, kitty_pid);
    }

    inline void stop() {
        serve::instance().stop();
    }
}