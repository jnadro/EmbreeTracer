#include "RenderKernels.h"
#include <embree2/rtcore.h>

extern "C" {
	extern void simple(float* vin, float* vout, int count);
	extern void calculateSceneColor(RTCScene scene, RTCRay* ray, int width, int height, uint8_t* gl_FragCoord);
}

void Simple(float* vin, float* vout, int count)
{
	simple(vin, vout, count);
}

void CalculateSceneColor(RTCScene scene, RTCRay* ray, int width, int height, unsigned char* gl_FragCoord)
{
	calculateSceneColor(scene, ray, width, height, gl_FragCoord);
}

