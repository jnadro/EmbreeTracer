#include <cassert>
#include <iostream>
#include <limits>

#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

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

	unsigned quad = rtcNewTriangleMesh2(scene, RTC_GEOMETRY_STATIC, 2, 4);
	{
		vec4* vertices = (vec4*)rtcMapBuffer(scene, quad, RTC_VERTEX_BUFFER);
		assert(vertices);
		vertices[0].x = 0.0f; vertices[0].y = 1.0f; vertices[0].z = -1.0f; vertices[0].a = 1.0f;
		vertices[1].x = 1.0f; vertices[1].y = 0.0f; vertices[1].z = -1.0f; vertices[1].a = 1.0f;
		vertices[2].x = 0.0f; vertices[2].y = 0.0f; vertices[2].z = -1.0f; vertices[2].a = 1.0f;
		vertices[3].x = 1.0f; vertices[3].y = 1.0f; vertices[3].z = -1.0f; vertices[3].a = 1.0f;
		rtcUnmapBuffer(scene, quad, RTC_VERTEX_BUFFER);

		rtcSetBuffer2(scene, quad, RTC_USER_VERTEX_BUFFER0, colors, 0, sizeof(vec4));
	}
	{
		Triangle* triangles = (Triangle*)rtcMapBuffer(scene, quad, RTC_INDEX_BUFFER);
		assert(triangles);
		triangles[0].v0 = 0; triangles[0].v1 = 1; triangles[0].v2 = 2;
		triangles[1].v0 = 0; triangles[1].v1 = 3; triangles[1].v2 = 1;
		rtcUnmapBuffer(scene, quad, RTC_INDEX_BUFFER);
	}

	rtcCommit(scene);

	// start tracing
	{
		unsigned width = 200;
		unsigned height = 200;
		const float aspectRatio = (float)width / height;

		PPMImage colorAOV(width, height);

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
					vec4 color;
					rtcInterpolate2(scene, cameraRay.geomID, cameraRay.primID, cameraRay.u, cameraRay.v, RTC_USER_VERTEX_BUFFER0, &color.x, nullptr, nullptr, nullptr, nullptr, nullptr, 4);
					colorAOV.SetPixel(x, y, color.x, color.y, color.z);
				}
				else
				{
					colorAOV.SetPixel(x, y, 0.5f, 0.5f, 0.5f);
				}
			}
		}

		colorAOV.Write("color.ppm");
	}

	rtcDeleteGeometry(scene, quad);
	rtcDeleteScene(scene);
	rtcDeleteDevice(device);

    return 0;
}