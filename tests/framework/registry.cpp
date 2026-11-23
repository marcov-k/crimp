#include "registry.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

std::vector<TestEntry>& Registry()
{
	static std::vector<TestEntry> registry{};
	return registry;
}

void RegisterTest(std::string name, std::function<void()> func)
{
	Registry().push_back({ std::move(name), std::move(func) });
}