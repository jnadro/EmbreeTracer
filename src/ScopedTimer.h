#pragma once
#include <string>
#include <windows.h>

class ScopedTimer
{
public:

	ScopedTimer(const std::string& InputMessage);
	ScopedTimer() = delete;
	~ScopedTimer();

private:
	double GetTime();

	LARGE_INTEGER counter;
	LARGE_INTEGER frequency;
	double time0 = 0.0;
	const std::string message;
};
