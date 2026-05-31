#pragma once
#include <unordered_set>
#include <vector>

#include "symbol.hpp"
#include "containers/hash_map.hpp"

namespace pars
{
	struct Node;

	struct AutoScope;

	constexpr auto PUBLIC_SYMBOL = true;
	constexpr auto PRIVATE_SYMBOL = false;

	class ScopeTable
	{
	public:
		using Scope = HashMap<std::string_view, Node*>;
		using Level = u16;

		static constexpr u16 UNLOCKED_LEVEL = UINT16_MAX;

		struct ScopeData
		{
			std::vector<u32> imports {};
			Scope scope;
		};

		ScopeTable();

		void set_file_id(u16 file_id);

		[[nodiscard]]
		u16 get_level() const
		{
			return m_level;
		}

		void set_lock(u16 level)
		{
			m_locked_level = level;
		}

		[[nodiscard]]
		AutoScope new_scope();

		void go_down();
		void go_up();
		Scope& get_scope();

		void clear_level(u16 level);

		void add_import(u32 file_id);

		void add_to_scope(Symbol symbol, Node *node, bool is_public = true, u16 level = UINT16_MAX);
		Node* find_symbol(std::string_view name) const;
		Node* find_local_symbol(std::string_view name) const;
		bool has_symbol(std::string_view name);

		template<IsNode T>
		T* find_symbol(std::string_view name)
		{
			auto *node = find_symbol(name);

			return dynamic_cast<T*>(node);
		}

		const std::vector<ScopeData>& get_table() const
		{
			return m_table;
		}

	private:
		u16 m_file_id {};
		u16 m_level {};
		u16 m_locked_level = UNLOCKED_LEVEL;
		std::vector<ScopeData> m_table;
		inline static std::unordered_set<u32> m_modules_found;
	};

	struct AutoScope
	{
		ScopeTable &table;

		explicit AutoScope(ScopeTable &scope);
		~AutoScope();
	};
}
