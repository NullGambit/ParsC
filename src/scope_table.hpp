#pragma once
#include <vector>

#include "symbol.hpp"
#include "containers/hash_map.hpp"

namespace pars
{
	struct Node;

	struct AutoScope;

	class ScopeTable
	{
	public:
		using Scope = HashMap<std::string_view, Node*>;

		ScopeTable();

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

		void add_to_scope(Symbol symbol, Node *node, u32 level = UINT32_MAX);
		void add_to_scope(std::string_view name, Node *node, u32 level = UINT32_MAX);
		Node* find_symbol(std::string_view name);
		bool has_symbol(std::string_view name);

		template<IsNode T>
		T* find_symbol(std::string_view name)
		{
			auto *node = find_symbol(name);

			return dynamic_cast<T*>(node);
		}

	private:
		u32 m_level {};
		std::vector<Scope> m_table;
	};

	struct AutoScope
	{
		ScopeTable &table;

		AutoScope(ScopeTable &scope);
		~AutoScope();
	};
}
