#include <iostream>

#include "ScopedTimer.h"

ScopedTimer::ScopedTimer(const std::string& InputMessage)
	: message(InputMessage)
{
	QueryPerformanceFrequency(&frequency);
	time0 = GetTime();
}

ScopedTimer::~ScopedTimer()
{
	std::cout << message << ": " << elapsed() << " ms.\n";
}

double ScopedTimer::elapsed()
{
	return (GetTime() - time0) * 1000.0;
}

double ScopedTimer::GetTime()
{
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart * (1.0 / (double)frequency.QuadPart);
}
