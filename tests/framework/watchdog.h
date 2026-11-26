#pragma once

#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

class Watchdog
{
public:
	Watchdog(std::chrono::steady_clock::duration timeLimit);

	Watchdog(const Watchdog&) = delete;
	Watchdog& operator=(const Watchdog&) = delete;
	Watchdog(Watchdog&&) = delete;
	Watchdog& operator=(Watchdog&&) = delete;

	void Begin(std::string_view testName);
	void End();

private:
	struct TestState
	{
		std::string testName{};
		std::chrono::time_point<std::chrono::steady_clock> startTime{};
		bool running{ false };
		std::size_t testCounter{ 0 };
	};

	TestState _state{};
	std::mutex _stateMutex{};
	std::chrono::steady_clock::duration _timeLimit;
	std::jthread _thread;
	
	void WatchdogLoop(std::stop_token stop);
};