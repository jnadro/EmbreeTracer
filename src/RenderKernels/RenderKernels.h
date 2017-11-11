#pragma once

#include <stdint.h>

#include <embree2/rtcore.h>

struct RTCRay;

extern "C" { 
	__declspec(dllexport) void Simple(float* vin, float* vout, int count);
	__declspec(dllexport) void CalculateSceneColor(RTCScene scene, RTCRay* ray, int width, int height, uint8_t* gl_FragCoord);
}
