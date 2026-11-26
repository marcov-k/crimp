#pragma once

#include "watchdog.h"

#include <string>

class WatchdogGuard
{
public:
	WatchdogGuard(Watchdog& watchdog, std::string testName) : _watchdog(watchdog)
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