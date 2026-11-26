#include "watchdog.h"

#include "error_codes.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <chrono>
#include <cstdio>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

Watchdog::Watchdog(std::chrono::steady_clock::duration timeLimit)
	: _timeLimit(timeLimit)
{
	_thread = std::jthread{ std::bind_front(&Watchdog::WatchdogLoop, this) };
}

void Watchdog::Begin(std::string testName)
{
	if (_timeLimit == std::chrono::steady_clock::duration::zero())
	{
		return;
	}

	{
		std::lock_guard lock{ _stateMutex };
		_state.testName = testName;
		_state.startTime = std::chrono::steady_clock::now();
		_state.running = true;
	}
}

void Watchdog::End()
{
	if (_timeLimit == std::chrono::steady_clock::duration::zero())
	{
		return;
	}

	{
		std::lock_guard lock{ _stateMutex };
		_state.running = false;
	}
}

void Watchdog::WatchdogLoop(std::stop_token stop)
{
	while (!stop.stop_requested())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::string name;
		{
			std::lock_guard lock{ _stateMutex };
			if (!_state.running) continue;

			auto elapsed{ std::chrono::steady_clock::now() - _state.startTime };
			if (elapsed < _timeLimit) continue;

			name = _state.testName;
		}
		
		if (IsDebuggerPresent())
		{
			{
				std::lock_guard lock{ _stateMutex };
				_state.startTime = std::chrono::steady_clock::now();
			}
			continue;
		}

		std::fprintf(stderr, "TIMEOUT: %s exceeded %lfs\n", name.c_str(), std::chrono::duration_cast<std::chrono::duration<double>>(_timeLimit).count());
		_exit(CRIMP_TEST_TIMEOUT);
	}
}