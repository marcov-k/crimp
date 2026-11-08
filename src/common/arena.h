#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

class Arena
{
public:
	Arena() = default;

	Arena(const Arena&) = delete;
	Arena& operator=(const Arena&) = delete;
	Arena(Arena&&) = delete;
	Arena& operator=(Arena&&) = delete;

	template<typename T, typename... Args>
	[[nodiscard]] T* Create(Args&&... args)
	{
		static_assert(std::is_trivially_destructible_v<T>, "arena types must be trivially destructible");

		void* mem{ Alloc(sizeof(T), alignof(T)) };

		return ::new (mem) T{ std::forward<Args>(args)... };
	}

	template<typename T>
	[[nodiscard]] T* CreateArray(std::size_t count)
	{
		static_assert(std::is_trivially_destructible_v<T>, "arena types must be trivially destructible");

		if (count == 0) return nullptr;
		assert(count < SIZE_MAX / sizeof(T));

		void* mem{ Alloc(sizeof(T) * count, alignof(T)) };
		T* ptr{ static_cast<T*>(mem) };
		std::uninitialized_value_construct_n(ptr, count);

		return ptr;
	}

	template<typename T>
	[[nodiscard]] T* CopyArray(std::span<const T> data)
	{
		static_assert(std::is_trivially_destructible_v<T>, "arena types must be trivially destructible");

		if (data.empty()) return nullptr;
		assert(data.size() < SIZE_MAX / sizeof(T));

		void* mem{ Alloc(sizeof(T) * data.size(), alignof(T)) };
		T* ptr{ static_cast<T*>(mem) };
		std::uninitialized_copy(data.begin(), data.end(), ptr);

		return ptr;
	}

private:
	static constexpr std::size_t BLOCK{ 64 * 1024 };

	std::vector<std::unique_ptr<char[]>> _blocks{};
	char* _cursor{ nullptr };
	char* _end{ nullptr };

	void* Alloc(std::size_t size, std::size_t alignment);
};