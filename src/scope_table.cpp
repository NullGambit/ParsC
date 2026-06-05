#include "scope_table.hpp"

#include <unordered_set>

#include "module_manager.hpp"

pars::ScopeTable::ScopeTable()
{
	m_table.resize(6);
}

void pars::ScopeTable::set_file_id(u16 file_id)
{
	m_file_id = file_id;
}

pars::AutoScope pars::ScopeTable::new_scope()
{
	return AutoScope{*this};
}

void pars::ScopeTable::go_down()
{
	m_level++;

	if (m_level >= m_table.size())
	{
		m_table.emplace_back();
	}

	get_scope_data(m_level).flags = {};
}

void pars::ScopeTable::go_up()
{
	if (m_level <= 0)
	{
		return;
	}

	clear_level(m_level);

	m_level--;
}

pars::ScopeTable::Scope& pars::ScopeTable::get_scope()
{
	return m_table[m_level].scope;
}

pars::ScopeFlags & pars::ScopeTable::get_lower_flags()
{
	return get_scope_data(m_level + 1).flags;
}

pars::ScopeFlags & pars::ScopeTable::get_current_flags()
{
	return get_scope_data(m_level).flags;
}

pars::ScopeTable::ScopeData & pars::ScopeTable::get_scope_data(u16 level)
{
	return m_table[level];
}

void pars::ScopeTable::clear_level(u16 level)
{
	if (level < m_table.size())
	{
		auto &[imports, scope, flags] = m_table[m_level];

		imports.clear();
		scope.clear();
		// flags = {};
	}
}

void pars::ScopeTable::add_import(u32 file_id)
{
	auto &data = m_table[m_level];

	data.imports.emplace_back(file_id);
}

void pars::ScopeTable::add_to_scope(Symbol symbol, Node *node, bool is_public, u16 level)
{
	if (m_level > m_locked_level && m_locked_level != UNLOCKED_LEVEL)
	{
		return;
	}

	Scope *scope;

	if (level != UINT16_MAX)
	{
		if (level >= m_table.size())
		{
			for (auto i = 0; i < level; i++)
			{
				m_table.emplace_back();
			}
		}

		scope = &m_table[level].scope;
	}
	else
	{
		scope = &get_scope();
	}

	scope->emplace(symbol.name, node);

	if (is_public && m_level == 0)
	{
		declare_global_symbol(symbol.name,
		{
			.node = node,
			.availability = GlobalSymbolAvailability::WhenImported,
			.file_id = m_file_id
		});
	}
}

pars::Node* pars::ScopeTable::find_symbol(std::string_view name) const
{
	std::span<GlobalSymbol> symbols;

	// if name starts with lowercase than perfect chance this is a builtin primitive type
	// therefore must be a global symbol
	if (!name.empty() && std::islower(name[0]))
	{
		symbols = find_global_symbol(name);

		for (auto &symbol : symbols)
		{
			if (symbol.availability == GlobalSymbolAvailability::Always)
			{
				return symbol.node;
			}
		}
	}

	auto *node = find_local_symbol(name);

	if (node != nullptr)
	{
		return node;
	}

	// TODO: allow for parameterized up to down or down to up scope checking
	for (auto i = 0; i <= m_level; i++)
	{
		auto &data = m_table[i];
		auto iter = data.scope.find(name);

		m_modules_found.insert(data.imports.begin(), data.imports.end());

		if (iter != data.scope.end())
		{
			return iter->second;
		}
	}

	if (symbols.empty())
	{
		symbols = find_global_symbol(name);
	}

	for (auto &symbol : symbols)
	{
		if (symbol.availability == GlobalSymbolAvailability::Always || m_modules_found.contains(symbol.file_id))
		{
			return symbol.node;
		}
	}

	return nullptr;
}

pars::Node * pars::ScopeTable::find_local_symbol(std::string_view name) const
{
	m_modules_found.clear();

	for (auto i = 0; i <= m_level; i++)
	{
		auto &data = m_table[i];
		auto iter = data.scope.find(name);

		m_modules_found.insert(data.imports.begin(), data.imports.end());

		if (iter != data.scope.end())
		{
			return iter->second;
		}
	}

	return nullptr;
}

bool pars::ScopeTable::has_symbol(std::string_view name)
{
	return find_symbol(name) != nullptr;
}

pars::AutoScope::AutoScope(ScopeTable &scope) :
	table{scope}
{
	scope.go_down();
}

pars::AutoScope::~AutoScope()
{
	table.go_up();
}
