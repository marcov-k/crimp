#include "scoped_handle.h"

#include <windows.h>

ScopedHandle::ScopedHandle(HANDLE h) : _h(h) {}

ScopedHandle::~ScopedHandle()
{
	Reset();
}

ScopedHandle::ScopedHandle(ScopedHandle&& o) noexcept : _h(o.Release()) {}

ScopedHandle& ScopedHandle::operator=(ScopedHandle&& o) noexcept
{
	if (this != &o) Reset(o.Release());
	return *this;
}

HANDLE ScopedHandle::Get() const
{
	return _h;
}

bool ScopedHandle::Valid() const
{
	return _h != nullptr && _h != INVALID_HANDLE_VALUE;
}

HANDLE ScopedHandle::Release()
{
	HANDLE t{ _h };
	_h = nullptr;
	return t;
}

void ScopedHandle::Reset(HANDLE h)
{
	if (Valid()) CloseHandle(_h);
	_h = h;
}