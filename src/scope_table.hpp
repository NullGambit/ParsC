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
	constexpr auto PRIVATE_SYMBOL = true;

	class ScopeTable
	{
	public:
		using Scope = HashMap<std::string_view, Node*>;

		struct ScopeData
		{
			std::vector<u32> imports {};
			Scope scope;
		};

		ScopeTable();

		void set_file_id(u16 file_id);

		[[nodiscard]]
		u32 get_level() const
		{
			return m_level;
		}

		[[nodiscard]]
		AutoScope new_scope();

		void go_down();
		void go_up();
		Scope& get_scope();

		void add_import(u32 file_id);

		void add_to_scope(Symbol symbol, Node *node, bool is_public = true, u32 level = UINT32_MAX);
		Node* find_symbol(std::string_view name);
		Node* find_local_symbol(std::string_view name);
		bool has_symbol(std::string_view name);

		template<IsNode T>
		T* find_symbol(std::string_view name)
		{
			auto *node = find_symbol(name);

			return dynamic_cast<T*>(node);
		}

	private:
		u16 m_file_id {};
		u32 m_level {};
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
