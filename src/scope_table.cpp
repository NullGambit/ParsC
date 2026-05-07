#include "scope_table.hpp"

pars::ScopeTable::ScopeTable()
{
	m_table.resize(6);
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
}

void pars::ScopeTable::go_up()
{
	m_table[m_level].clear();
	m_level--;
}

pars::ScopeTable::Scope& pars::ScopeTable::get_scope()
{
	return m_table[m_level];
}

void pars::ScopeTable::add_to_scope(Symbol symbol, Node *node, u32 level)
{
	add_to_scope(symbol.name, node, level);
}

void pars::ScopeTable::add_to_scope(std::string_view name, Node *node, u32 level)
{
	Scope *scope;

	if (level != UINT32_MAX)
	{
		if (level >= m_table.size())
		{
			for (u32 i = 0; i < level; i++)
			{
				m_table.emplace_back();
			}
		}

		scope = &m_table[level];
	}
	else
	{
		scope = &get_scope();
	}

	scope->emplace(name, node);
}

pars::Node* pars::ScopeTable::find_symbol(std::string_view name)
{
	// TODO: allow for parameterized up to down or down to up scope checking
	for (auto i = 0; i <= m_level; i++)
	{
		auto &scope = m_table[i];
		auto iter = scope.find(name);

		if (iter != scope.end())
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
