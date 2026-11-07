#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

class ScopedHandle
{
public:
	ScopedHandle() = default;
	explicit ScopedHandle(HANDLE h);
	~ScopedHandle();

	ScopedHandle(const ScopedHandle&) = delete;
	ScopedHandle& operator=(const ScopedHandle&) = delete;
	ScopedHandle(ScopedHandle&& o) noexcept;
	ScopedHandle& operator=(ScopedHandle&& o) noexcept;

	HANDLE Get() const;
	bool Valid() const;
	HANDLE Release();
	void Reset(HANDLE h = nullptr);

private:
	HANDLE _h{ nullptr };
};