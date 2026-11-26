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
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <string_view>
#include <thread>

Watchdog::Watchdog(std::chrono::steady_clock::duration timeLimit)
	: _timeLimit(timeLimit)
{
	if (_timeLimit > std::chrono::steady_clock::duration::zero())
	{
		_thread = std::jthread{ [this](std::stop_token stop) { WatchdogLoop(stop); } };
	}
}

void Watchdog::Begin(std::string_view testName)
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
		++_state.testCounter;
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
		std::size_t testCounter;
		{
			std::lock_guard lock{ _stateMutex };
			if (!_state.running) continue;

			auto elapsed{ std::chrono::steady_clock::now() - _state.startTime };
			if (elapsed < _timeLimit) continue;

			name = _state.testName;
			testCounter = _state.testCounter;
		}
		
		if (IsDebuggerPresent())
		{
			{
				std::lock_guard lock{ _stateMutex };
				if (_state.testCounter == testCounter)
				{
					_state.startTime = std::chrono::steady_clock::now();
				}
			}
			continue;
		}

		std::fprintf(stderr, "TIMEOUT: %s exceeded %gs\n", name.c_str(), std::chrono::duration_cast<std::chrono::duration<double>>(_timeLimit).count());
		_exit(CRIMP_TEST_TIMEOUT);
	}
}