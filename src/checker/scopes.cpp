#include "scopes.h"

#include "names.h"

#include <cassert>
#include <cstddef>
#include <vector>

void Scopes::Push()
{
	_scopes.emplace_back();
}

void Scopes::Pop()
{
	assert(!_scopes.empty());

	_scopes.pop_back();
}

Symbol* Scopes::Declare(NameId id, Symbol* symbol)
{
	assert(!_scopes.empty());

	auto res{ _scopes.back().try_emplace(id, symbol) };
	if (!res.second) return res.first->second; // if id found return existing symbol

	return nullptr;
}

Symbol* Scopes::Lookup(NameId id) const
{
	for (std::size_t i{ _scopes.size() }; i > 0; --i)
	{
		const auto& scope{ _scopes[i - 1] };

		auto found{ scope.find(id) };
		if (found != scope.end())
		{
			return found->second;
		}
	}

	return nullptr;
}

std::size_t Scopes::Depth() const
{
	return _scopes.size();
}