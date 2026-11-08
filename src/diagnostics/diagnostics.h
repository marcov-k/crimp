#pragma once

#include "source_map.h"

#include "../common/span.h"

#include <string>
#include <vector>

enum class Severity
{
	Error,
	Warning,
	Note
};

struct Note
{
	Span span;
	std::string message;
};

struct Diagnostic
{
	Severity severity;
	Span span;
	std::string message;
	std::vector<Note> notes;
};

class Diagnostics
{
public:
	explicit Diagnostics(size_t maxErrors = 20);

	void Emit(Diagnostic d);
	
	void Error(Span s, std::string msg);

	void Warning(Span s, std::string msg);

	bool HasErrors() const;

	bool LimitReached() const;

	const std::vector<Diagnostic>& All() const;

private:
	std::vector<Diagnostic> _list;
	size_t _errors{ 0 };
	size_t _maxErrors;
};

std::string Render(const SourceMap& sm, const Diagnostic& d);