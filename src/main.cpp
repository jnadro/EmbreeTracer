#include <cassert>
#include <iostream>
#include <limits>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include "Mesh.h"
#include "PPMImage.h"

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

struct vec4 { float x, y, z, a; };
struct Triangle { int v0, v1, v2; };

vec4 colors[4] = {
	{ 0.0f, 0.0f, 0.0f, 1.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f } };

struct vec3 
{ 
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
};

static RTCRay makeRay(const vec3& org, const vec3& dir)
{
	RTCRay ray{};
	ray.org[0] = org.x;
	ray.org[1] = org.y;
	ray.org[2] = org.z;
	ray.dir[0] = dir.x;
	ray.dir[1] = dir.y;
	ray.dir[2] = dir.z;
	ray.tnear = 0.0f;
	ray.tfar = std::numeric_limits<float>::max();
	ray.time = 0.0f;
	ray.mask = 0;
	ray.geomID = RTC_INVALID_GEOMETRY_ID;
	ray.primID = RTC_INVALID_GEOMETRY_ID;
	return ray;
}

float dot(const float row[4], vec3 v)
{
	return (row[0] * v.x + row[1] * v.y + row[2] * v.z + row[3] * 1.0f);
}

void translate(float matrix[4][4], const vec3& translation)
{
	matrix[0][3] = translation.x;
	matrix[1][3] = translation.y;
	matrix[2][3] = translation.z;
}

int main(int argc, char* argv[])
{
	RTCDevice device = rtcNewDevice();
	EmbreeErrorHandler(nullptr, rtcDeviceGetError(nullptr), nullptr);

	rtcDeviceSetErrorFunction2(device, EmbreeErrorHandler, nullptr);

	RTCScene scene = rtcDeviceNewScene(device, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERPOLATE);

	TriangleMesh* Sphere = LoadObjMesh("quad.obj", scene);
	assert(Sphere);

	rtcCommit(scene);

	// start tracing
	{
		unsigned width = 800;
		unsigned height = 800;
		const float aspectRatio = (float)width / height;

		PPMImage colorAOV(width, height);
		PPMImage uvAOV(width, height);
		PPMImage normalAOV(width, height);

		vec3 origin{ 0.0f, 0.0f, 0.0f };

		float CameraToWorld[4][4]{
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 0.0f, 1.0f}
		};
		translate(CameraToWorld, vec3{0.0f, 0.0f, 2.0f});

		for (unsigned y = 0; y < height; ++y)
		{
			for (unsigned x = 0; x < width; ++x)
			{
				float pixelNDCX = ((float)x + 0.5f) / width;
				float pixelNDCY = ((float)y + 0.5f) / height;
				
				float Px = (2.0f * pixelNDCX - 1.0f) * aspectRatio;
				float Py = (1.0f - 2.0f * pixelNDCY);

				vec3 rayP{Px, Py, -1.0f};

				vec3 rayWorldOrigin{ dot(CameraToWorld[0], origin), dot(CameraToWorld[1], origin), dot(CameraToWorld[2], origin) };
				vec3 rayPWorld{ dot(CameraToWorld[0], rayP), dot(CameraToWorld[1], rayP), dot(CameraToWorld[2], rayP) };

				vec3 rayWorldDir{
					rayPWorld.x - rayWorldOrigin.x,
					rayPWorld.y - rayWorldOrigin.y,
					rayPWorld.z - rayWorldOrigin.z
				};

				RTCRay cameraRay = makeRay(rayWorldOrigin, rayWorldDir);
				rtcIntersect(scene, cameraRay);
				if (cameraRay.geomID != RTC_INVALID_GEOMETRY_ID)
				{
					vec4 color{ 1.0f, 0.0f, 0.04f };
					colorAOV.SetPixel(x, y, color.x, color.y, color.z);

					vec4 uv{ 0.0f, 0.0f, 0.0f, 0.0f };
					rtcInterpolate2(scene, cameraRay.geomID, cameraRay.primID, cameraRay.u, cameraRay.v, RTC_USER_VERTEX_BUFFER0, &uv.x, nullptr, nullptr, nullptr, nullptr, nullptr, 2);
					uvAOV.SetPixel(x, y, uv.x, uv.y, 0.0f);

					vec4 n{ 0.0f, 0.0f, 0.0f, 0.0f };
					rtcInterpolate2(scene, cameraRay.geomID, cameraRay.primID, cameraRay.u, cameraRay.v, RTC_USER_VERTEX_BUFFER1, &n.x, nullptr, nullptr, nullptr, nullptr, nullptr, 3);
					normalAOV.SetPixel(x, y, n.x, n.y, n.z);
				}
				else
				{
					vec4 backgroundColor{ 0.5f, 0.5f, 0.5f, 1.0f };
					colorAOV.SetPixel(x, y, backgroundColor.x, backgroundColor.y, backgroundColor.z);
					uvAOV.SetPixel(x, y, backgroundColor.x, backgroundColor.y, backgroundColor.z);
					normalAOV.SetPixel(x, y, backgroundColor.x, backgroundColor.y, backgroundColor.z);
				}
			}
		}

		colorAOV.Write("color.tga");
		uvAOV.Write("uv.tga");
		normalAOV.Write("normal.tga");
	}

	rtcDeleteScene(scene);
	rtcDeleteDevice(device);

    return 0;
}