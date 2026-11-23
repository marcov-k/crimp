#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

struct TestEntry
{
	std::string name;
	std::function<void()> func;
};

std::vector<TestEntry>& Registry();

void RegisterTest(std::string name, std::function<void()> func);

struct TestRegistrar
{
	TestRegistrar(std::string name, std::function<void()> func)
	{
		RegisterTest(std::move(name), std::move(func));
	}
};