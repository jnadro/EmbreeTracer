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
	const double elapsed = (GetTime() - time0) * 1000.0;
	std::cout << message << ": " << elapsed << " ms.\n";
}

double ScopedTimer::GetTime()
{
	QueryPerformanceCounter(&counter);
	return (double)counter.QuadPart * (1.0 / (double)frequency.QuadPart);
}
