#include "RenderKernels.h"

extern "C" {
	extern void simple(float* vin, float* vout, int count);
}

void Simple(float* vin, float* vout, int count)
{
	simple(vin, vout, count);
}
