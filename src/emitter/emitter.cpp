#include "emitter.h"

#include "../ast/ast.h"
#include "../checker/symbols.h"
#include "../diagnostics/source_map.h"

#include <string>
#include <unordered_map>

namespace
{
	struct EmitterState
	{
		const SourceMap& source;
		std::unordered_map<Symbol*, std::string> symbolMap{};
	};
}

std::string EmitIL(const SourceMap& source, const AstProgram* program)
{
	return {};
}