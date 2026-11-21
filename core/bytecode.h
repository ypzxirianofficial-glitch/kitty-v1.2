#pragma once

#include <Luau/Compiler/include/luacode.h>
#include <Luau/Bytecode.h>
#include <Luau/BytecodeBuilder.h>
#include <Luau/Common.h>
#include <Luau/Compiler.h>
#include <Luau/BytecodeUtils.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include <signer/include/signer.h>
#include <zstd_xxhash/include/zstd.h>
#include <zstd_xxhash/include/xxhash.h>

namespace bytecode {
    inline void repStr(std::string& source, const std::string_view toReplace, const std::string_view replacement) {
        size_t pos = source.find(toReplace);
        if (pos != std::string::npos) source.replace(pos, toReplace.length(), replacement);
    }

    class bytecode_encoder_t : public Luau::BytecodeEncoder {
        inline void encode(uint32_t* data, size_t count) override {
            for (auto i = 0u; i < count;) {
                auto& opcode = *reinterpret_cast<uint8_t*>(data + i);
                i += Luau::getOpLength(LuauOpcode(opcode));
                opcode *= 227;
            }
        }
    };

    inline std::string compress(const std::string& Bytecode) {
        size_t DataSize = Bytecode.size();
        size_t MaxSize = ZSTD_compressBound(DataSize);
        std::vector<char> Buffer(MaxSize + 8);

        uint32_t DataSize32 = (uint32_t)DataSize;

        memcpy(Buffer.data(), "RSB1", 4);
        memcpy(Buffer.data() + 4, &DataSize32, sizeof(DataSize32));

        size_t CompressedSize = ZSTD_compress(Buffer.data() + 8, MaxSize, Bytecode.data(), DataSize, ZSTD_maxCLevel());
        size_t TotalSize = CompressedSize + 8;

        uint32_t Key = XXH32(Buffer.data(), TotalSize, 42);
        uint8_t* KeyBytes = (uint8_t*)&Key;

        for (size_t i = 0; i < TotalSize; ++i)
            Buffer[i] ^= KeyBytes[i % 4] + (uint8_t)(i * 41);

        return std::string(Buffer.data(), TotalSize);
    }

    inline std::string compile(const std::string& source)
    {
        static bytecode_encoder_t encoder = bytecode_encoder_t();

        static const char* CommonGlobals[] = {
            "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", "_G", "_ENV",
            "task", "bit32", "buffer", "debug", "warn", "typeof", "getfenv", "setfenv",
            nullptr
        };

        Luau::CompileOptions Options;
        Options.debugLevel = 1;
        Options.optimizationLevel = 1;
        Options.mutableGlobals = CommonGlobals;
        Options.vectorLib = "Vector3";
        Options.vectorCtor = "new";
        Options.vectorType = "Vector3";

        const std::string bytecode = Luau::compile(source, Options, {}, &encoder);

        if (!bytecode.empty()) {
            if (bytecode[0] == '\0') {
                std::string bytecodeP = bytecode;
                bytecodeP.erase(std::remove(bytecodeP.begin(), bytecodeP.end(), '\0'), bytecodeP.end());
            }
        }

        return bytecode;
    }

    inline signing::signer signer;

    inline auto __c(std::vector<uint8_t> bytecode) { return std::string(bytecode.begin(), bytecode.end()); }
    inline auto __c(std::string bytecode) { return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(bytecode.data()), bytecode.size()); }

    inline auto sign(std::string bytecode) { return signer.sign(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(bytecode.data()), bytecode.size())); }

    inline bool verify(std::string bytecode) { return signer.verify(__c(bytecode)); }
    inline bool verify(std::vector<uint8_t> bytecode) { return signer.verify(bytecode); }

    // thanks to @byteblue for fix
    inline std::vector<uint8_t> prepare(std::string source)
    {
        // fix unicode shi leaking through
        std::string bom = "\xEF\xBB\xBF";
        size_t pos = 0;
        while ((pos = source.find(bom, pos)) != std::string::npos) {
            source.erase(pos, bom.length());
        }
        // added debug for people to see its there bytecode unicode
        std::string compiled = compile(source);
        if (compiled.empty() || compiled[0] == '\0') {
            std::string err = compiled;
            err.erase(std::remove(err.begin(), err.end(), '\0'), err.end());
            std::cout << "\n[Bytecode Compile Error]\n" << err << "\n" << std::endl;
            return {};
        }

        auto signed_bc = sign(compiled);
        if (signed_bc.empty()) {
            std::cout << "[Bytecode] Signing Error!" << std::endl;
            return {};
        }

        std::string compressed = compress(__c(signed_bc));
        if (compressed.empty()) {
            std::cout << "[Bytecode] Compression Error!" << std::endl;
            return {};
        }

        return std::vector<uint8_t>(compressed.begin(), compressed.end());
    }
}