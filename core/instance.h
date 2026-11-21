#pragma once

#include "../kitty/kitty.h"
#include "offsets.h"

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace rbx {
    class instance {
    public:
        uintptr_t address = 0;

        instance() = default;
        instance(uintptr_t addr) : address(addr) {}
        instance(const instance&) = default;
        instance& operator=(const instance&) = default;

        explicit operator bool() const { return address != 0; }

        std::string name() {
            if (!address) return {};
            uintptr_t container = kitty.read<uintptr_t>(address + offsets::instance::name);
            if (!container) return {};
            uintptr_t ptr = container + offsets::instance::name_str;
            uintptr_t data = kitty.read<uintptr_t>(ptr);
            if (data) {
                std::string s = kitty.read_string(data);
                if (!s.empty()) return s;
            }
            std::string s = kitty.read_string(ptr);
            return s;
        }

        std::string class_name() {
            if (!address) return {};
            uintptr_t desc = kitty.read<uintptr_t>(address + offsets::instance::class_desc);
            if (!desc) return {};
            uintptr_t name_ptr = kitty.read<uintptr_t>(desc + offsets::instance::class_name);
            if (!name_ptr) return {};
            return kitty.read_string(name_ptr, 128);
        }

        instance find_first_child(const std::string& target) const {
            for (auto& c : get_children()) {
                if (static_cast<std::string>(c.name()) == target)
                    return c;
            }
            return instance{};
        }

        instance find_first_child_of_class(const std::string& target) const {
            for (auto& c : get_children()) {
                if (static_cast<std::string>(c.class_name()) == target)
                    return c;
            }
            return instance{};
        }

        std::vector<instance> get_children() const {
            std::vector<instance> out;
            if (!address) return out;

            uintptr_t container = kitty.read<uintptr_t>(address + offsets::instance::children_start);
            if (!container) return out;

            uintptr_t begin = kitty.read<uintptr_t>(container);
            uintptr_t end = kitty.read<uintptr_t>(container + offsets::instance::children_end);
            if (!begin || !end || end <= begin) return out;

            size_t count = (end - begin) / sizeof(uintptr_t);
            if (count > 0x4000) return out;

            out.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                uintptr_t child = kitty.read<uintptr_t>(begin + i * sizeof(uintptr_t));
                if (child) out.emplace_back(child);
            }
            return out;
        }

        std::vector<uintptr_t> get_children_addrs() const {
            std::vector<uintptr_t> out;
            for (auto& c : get_children()) out.push_back(c.address);
            return out;
        }

        instance parent() const {
            if (!address) return instance{};
            return instance{ kitty.read<uintptr_t>(address + 0x68) };
        }

        instance find(const std::vector<std::string>& path) const {
            instance cur = *this;
            for (const auto& part : path) {
                if (!cur) return instance{};
                cur = cur.find_first_child(part);
            }
            return cur;
        }

        instance value() const {
            return kitty.read<instance>(address + offsets::instance::value_base);
        }

        instance local_player() const {
            return kitty.read<instance>(address + offsets::instance::local_player);
        }

        template <typename instance_template>
        bool set_value(const instance_template& val) const {
            return kitty.write<instance_template>(address + offsets::instance::value_base, val);
        }

        bool spoof_to(instance target) const {
            return kitty.write<uintptr_t>(address+0x8, target.address) == TRUE;
        }

        bool operator==(const instance& o) const { return address == o.address; }
        bool operator!=(const instance& o) const { return address != o.address; }
    };

    class script_context : public instance {
    public:
        script_context() = default;
        script_context(uintptr_t addr) : instance(addr) {}
        script_context(const instance& inst) : instance(inst) {}

        script_context get_script_context() {
            uintptr_t children = kitty.read<uintptr_t>(address + offsets::instance::children_start);
            if (!children)
                return script_context(0);
            uintptr_t first_child = kitty.read<uintptr_t>(children);
            if (!first_child)
                return script_context(0);
            return script_context(kitty.read<uintptr_t>(first_child + offsets::datamodel::script_context));
        }

        bool set_bypass(bool t) {
            if (kitty.write<bool>(address + offsets::datamodel::require_bypass, t))
                return true;
            return false;
        }
    };

    class script : public instance {
    public:
        script() = default;
        script(uintptr_t addr) : instance(addr) {}
        script(const instance& inst) : instance(inst) {}

        std::vector<uint8_t> get_authentic_bytecode() const {
            uintptr_t offset = offsets::script::modulescript::pointer;
            if (rbx::instance(address).class_name() == "LocalScript")
                offset = offsets::script::localscript::pointer;

            uintptr_t ptr = kitty.read<uintptr_t>(address + offset);
            if (!ptr) return {};

            uintptr_t buffer = kitty.read<uintptr_t>(ptr + offsets::script::pointer);
            uintptr_t size = kitty.read<uintptr_t>(ptr + offsets::script::size);

            if (!buffer || !size || size > 0x10000000) return {};

            return kitty.read_bytes(buffer, size);
        }

        std::function<void()> set_bytecode(const std::vector<uint8_t>& bytecode) const {
            if (!address) return [] {};

            uintptr_t ptr = kitty.read<uintptr_t>(address + offsets::script::modulescript::pointer);
            if (!ptr) return [] {};

            uintptr_t old_buf = kitty.read<uintptr_t>(ptr + offsets::script::pointer);
            uintptr_t old_size = kitty.read<uintptr_t>(ptr + offsets::script::size);

            uintptr_t new_buf = kitty.allocate(bytecode.size());
            if (!new_buf) return [] {};

            if (!kitty.write_raw(new_buf, bytecode.data(), bytecode.size())) {
                kitty.free(new_buf);
                return [] {};
            }

            std::vector<uint8_t> verify(bytecode.size());
            if (!kitty.read_raw(new_buf, verify.data(), verify.size()) ||
                verify != bytecode) {
                kitty.free(new_buf);
                return [] {};
            }

            kitty.write<uintptr_t>(ptr + offsets::script::pointer, new_buf);
            kitty.write<uintptr_t>(ptr + offsets::script::size, bytecode.size());

            return [ptr, old_buf, old_size, new_buf]() {
                kitty.write<uintptr_t>(ptr + offsets::script::pointer, old_buf);
                kitty.write<uintptr_t>(ptr + offsets::script::size, old_size);
                kitty.free(new_buf);
                };
        }

        bool unlock_module() {
            kitty.write<uintptr_t>(address + offsets::script::modulescript::detection, 0x100000000);
            kitty.write<uintptr_t>(address + offsets::script::modulescript::core_script, 0x2);
            return true;
        }

        static script from(const instance& inst) {
            return script(inst.address);
        }
    };
}