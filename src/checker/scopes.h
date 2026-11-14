#pragma once

#include "names.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

struct Symbol;

class Scopes
{
public:
	void Push();
	void Pop();

	Symbol* Declare(NameId id, Symbol* symbol); // returns nullptr on success, existing symbol on a duplicate

	Symbol* Lookup(NameId id) const;

	std::size_t Depth() const;

private:
	std::vector<std::unordered_map<NameId, Symbol*>> _scopes{};
};

class ScopeGuard
{
public:
	explicit ScopeGuard(Scopes& scopes) : _scopes(scopes)
	{
		_scopes.Push();
	}

	~ScopeGuard()
	{
		_scopes.Pop();
	}

	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;
	ScopeGuard(ScopeGuard&&) = delete;
	ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
	Scopes& _scopes;
};