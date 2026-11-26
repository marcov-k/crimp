#pragma once

#include "watchdog.h"

#include <string_view>

class WatchdogGuard
{
public:
	WatchdogGuard(Watchdog& watchdog, std::string_view testName) : _watchdog(watchdog)
	{
		_watchdog.Begin(testName);
	}

	~WatchdogGuard()
	{
		_watchdog.End();
	}

	WatchdogGuard(const WatchdogGuard&) = delete;
	WatchdogGuard& operator=(const WatchdogGuard&) = delete;
	WatchdogGuard(WatchdogGuard&&) = delete;
	WatchdogGuard& operator=(WatchdogGuard&&) = delete;

private:
	Watchdog& _watchdog;
};