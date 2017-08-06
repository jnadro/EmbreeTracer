#include <cassert>
#include <iostream>
#include <limits>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

static void EmbreeErrorHandler(void* userPtr, const RTCError code, const char* str)
{
	if (code != RTC_NO_ERROR)
	{
		switch (code)
		{
		case RTC_UNKNOWN_ERROR: std::cout << "RTC_UNKNOWN_ERROR"; break;
		case RTC_INVALID_ARGUMENT: std::cout << "RTC_INVALID_ARGUMENT"; break;
		case RTC_INVALID_OPERATION: std::cout << "RTC_INVALID_OPERATION"; break;
		case RTC_OUT_OF_MEMORY: std::cout << "RTC_OUT_OF_MEMORY"; break;
		case RTC_UNSUPPORTED_CPU: std::cout << "RTC_UNSUPPORTED_CPU"; break;
		case RTC_CANCELLED: std::cout << "RTC_CANCELLED"; break;
		default: std::cout << "Invalid Error Code"; break;
		}

		if (str)
		{
			std::cout << " (" << str << ")\n";
		}
	}
}

struct Vertex { float x, y, z, a; };
struct Triangle { int v0, v1, v2; };

int main(int argc, char* argv[])
{
	RTCDevice device = rtcNewDevice();
	EmbreeErrorHandler(nullptr, rtcDeviceGetError(nullptr), nullptr);

	rtcDeviceSetErrorFunction2(device, EmbreeErrorHandler, nullptr);

	RTCScene scene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1);

	unsigned geomID = rtcNewTriangleMesh2(scene, RTC_GEOMETRY_STATIC, 1, 3);
	{
		Vertex* vertices = (Vertex*)rtcMapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
		assert(vertices);
		vertices[0].x = 0.0f; vertices[0].y = 1.0f; vertices[0].z = -2.0f; vertices[0].a = 1.0f;
		vertices[1].x = 1.0f; vertices[1].y = 0.0f; vertices[1].z = -2.0f; vertices[1].a = 1.0f;
		vertices[2].x = 0.0f; vertices[2].y = 0.0f; vertices[2].z = -2.0f; vertices[2].a = 1.0f;
		rtcUnmapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
	}
	{
		Triangle* triangles = (Triangle*)rtcMapBuffer(scene, geomID, RTC_INDEX_BUFFER);
		assert(triangles);
		triangles[0].v0 = 0; triangles[0].v1 = 1; triangles[0].v2 = 2;
		rtcUnmapBuffer(scene, geomID, RTC_INDEX_BUFFER);
	}

	rtcCommit(scene);

	RTCRay cameraRay;
	cameraRay.org[0] = 0.0f;
	cameraRay.org[1] = 0.0f;
	cameraRay.org[2] = 0.0f;

	cameraRay.dir[0] = 0.0f;
	cameraRay.dir[1] = 0.0f;
	cameraRay.dir[2] = -1.0f;

	cameraRay.tnear = 0.0f;
	cameraRay.tfar = std::numeric_limits<float>::max();
	cameraRay.time = 0.0f;
	cameraRay.mask = 0;
	cameraRay.geomID = RTC_INVALID_GEOMETRY_ID;
	cameraRay.primID = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect(scene, cameraRay);

	if (cameraRay.geomID == geomID)
	{
		std::cout << "Ray Hit!\n";
	}

	rtcDeleteGeometry(scene, geomID);
	rtcDeleteScene(scene);
	rtcDeleteDevice(device);

    return 0;
}