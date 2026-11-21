#pragma once
#include <windows.h>

namespace offsets {
	namespace task_scheduler {
		static constexpr uintptr_t target_fps = 0x85dcc40; // targetfps fast flag to setfpscap/getfpscap
	}
	namespace lua_thread { // need to decrypt lua_Thread* of ModuleScript
		inline const uintptr_t ref = 0x10; // ThreadRef pointer
		inline const uintptr_t control = 0x18; // control-block
		inline const uintptr_t thread = 0x28; // lua_Thread*
		inline const uintptr_t thread_ref_id = 0x30; // lua_Thread* id
		inline const uintptr_t Value_Ref_Id = 0x34; // lua_Thread* value
	}
	namespace datamodel {
		static constexpr uintptr_t pointer = 0x8dc8000;
		static constexpr uintptr_t real = 0x1f8;
		static constexpr uintptr_t script_context = 0x440;
		static constexpr uintptr_t require_bypass = 0xA81;
		static constexpr uintptr_t game_loaded = 0x5d0;
		static constexpr uintptr_t place_id = 0x188;
		static constexpr uintptr_t game_id = 0x180;
	}
	namespace instance {
		static constexpr uintptr_t name = 0x70;
		static constexpr uintptr_t name_str = 0x8;
		static constexpr uintptr_t class_desc = 0x18;
		static constexpr uintptr_t class_name = 0x8;
		static constexpr uintptr_t children_start = 0x78;
		static constexpr uintptr_t children_end = 0x8;
		static constexpr uintptr_t value_base = 0xa8;
		static constexpr uintptr_t local_player = 0x120;
	}
	namespace script {
		namespace localscript {
			static constexpr uintptr_t pointer = 0x180;
		}
		namespace modulescript {
			static constexpr uintptr_t module_state = 0x160;
			static constexpr uintptr_t pointer = 0x128;
			static constexpr uintptr_t core_script = 0x158;
			static constexpr uintptr_t detection = 0x158 - 0x4;
		}
		static constexpr uintptr_t pointer = 0x18;
		static constexpr uintptr_t size = 0x28;
	}
}