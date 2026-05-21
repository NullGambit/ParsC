#pragma once

#include "file_manager.hpp"
#include "scope_table.hpp"

namespace pars
{
	// shared data needed by all parts of the frontend
	struct ParseCtx
	{
		ScopeTable scope_table;
		SourceFile source_file;
	};
}
