#include <algorithm>
#include <cmath>
#include <random>

#include "Renderer.h"
#include "PPMImage.h"

vec3 WorldGetBackground(const RTCRay& ray)
{
	return vec3{ 0.0f, 0.0f, 0.0f };
}

static constexpr float PI = 3.14159265359f;
static const float Epsilon = 0.001f;
static const float gamma = 2.2f;

class RandomSample
{
	std::default_random_engine generator;
	std::uniform_real_distribution<float> distribution;

public:
	RandomSample(uint32_t seed) : generator(seed), distribution(0.0f, 1.0f) {}
	float next() { return distribution(generator); }
};

typedef vec3 Radiance;

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
	ray.mask = -1;
	ray.geomID = RTC_INVALID_GEOMETRY_ID;
	ray.primID = RTC_INVALID_GEOMETRY_ID;
	return ray;
}

static bool intersectScene(RTCScene scene, RTCRay& ray)
{
	rtcIntersect(scene, ray);
	if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		return true;
	}

	return false;
}

static Radiance shade(const std::vector<Material>& Materials, const RTCRay& ray)
{
	Radiance color(Materials[ray.geomID].DiffuseColor[0], Materials[ray.geomID].DiffuseColor[1], Materials[ray.geomID].DiffuseColor[2]);
	return color / PI;
}

float visibility(RTCScene scene, const vec3& o, const vec3& d)
{
	RTCRay shadowRay = makeRay(o, d);
	shadowRay.tnear = 0.001f;
	shadowRay.tfar = 1.0f;
	rtcOccluded(scene, shadowRay);
	return shadowRay.geomID ? 1.0f : 0.0f;
}

static vec3 uniformSampleHemisphere(RandomSample& sampler)
{
	const float r1 = sampler.next();
	const float r2 = sampler.next();
	const float sinTheta = std::sqrtf(1.0f - r1 * r1);
	const float phi = 2.0f * PI * r2;
	const float x = sinTheta * std::cosf(phi);
	const float z = sinTheta * std::sinf(phi);
	return normalize(vec3(x, r1, z));
}

static void createCoordinateSystem(const vec3& N, vec3& Nt, vec3& Nb)
{
	if (std::fabs(N.x) > std::fabs(N.y))
	{
		Nt = vec3(N.z, 0, -N.x) / std::sqrtf(N.x * N.x + N.z * N.z);
	}
	else
	{
		Nt = vec3(0, -N.z, N.y) / std::sqrtf(N.y * N.y + N.z * N.z);
	}

	Nb = cross(N, Nt);
}

static vec3 getBRDFRay(const vec3& P, const vec3& N, RandomSample& sampler)
{
	const vec3 hemisphereSample = uniformSampleHemisphere(sampler);
	vec3 Nt(0.0f, 0.0f, 0.0f), Nb(0.0f, 0.0f, 0.0f);
	createCoordinateSystem(N, Nt, Nb);
	vec3 sampleWorld(
		hemisphereSample.x * Nb.x + hemisphereSample.y * N.x + hemisphereSample.z * Nt.x,
		hemisphereSample.x * Nb.y + hemisphereSample.y * N.y + hemisphereSample.z * Nt.y,
		hemisphereSample.x * Nb.z + hemisphereSample.y * N.z + hemisphereSample.z * Nt.z);
	return sampleWorld;
}

static Radiance pathTraceRayRecursive(RTCScene scene, const std::vector<Material>& Materials, RTCRay& ray, RandomSample& sampler, uint32_t bounces)
{
	if (bounces == 0)
	{
		return Radiance(0.0f, 0.0f, 0.0f);
	}

	Radiance outgoing = Radiance(0.0f, 0.0f, 0.0f);
	if (intersectScene(scene, ray))
	{
		// intersection location
		vec3 P(ray.org[0] + ray.dir[0] * ray.tfar, ray.org[1] + ray.dir[1] * ray.tfar, ray.org[2] + ray.dir[2] * ray.tfar);
		vec3 Q(0.0f, 1.4f, 0.0f);
		vec3 toLight = Q - P;
		vec3 Wi = normalize(toLight);

		vec3 N(0.0f, 0.0f, 0.0f);
		rtcInterpolate2(scene, ray.geomID, ray.primID, ray.u, ray.v, RTC_USER_VERTEX_BUFFER1, &N.x, nullptr, nullptr, nullptr, nullptr, nullptr, 3);
		N = normalize(N);

		vec3 Power = vec3(1.0f, 1.0f, 1.0f);
		const float distance = toLight.length();
		vec3 DirectLighting = Power / (distance * distance) * visibility(scene, P, toLight) * std::max(0.0f, dot(N, Wi));

		constexpr float pdf = 1.0f / (2.0f * PI);

		vec3 worldDirection = getBRDFRay(P, N, sampler);
		vec3 IndirectLighting = pathTraceRayRecursive(scene, Materials, makeRay(P + worldDirection * Epsilon, worldDirection), sampler, bounces - 1) / pdf * std::max(0.0f, dot(N, normalize(worldDirection)));

		outgoing += (DirectLighting + IndirectLighting) * shade(Materials, ray);
	}
	else
	{
		outgoing += WorldGetBackground(ray);
	}

	return outgoing;
}

static RTCRay makeCameraRay(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	const float pixelNDCX = ((float)x + 0.5f) / width;
	const float pixelNDCY = ((float)y + 0.5f) / height;

	constexpr float fovAngle = (34.5159f / 2.0f) * (PI / 180.0f);
	const float fov = tan(fovAngle);

	const float aspectRatio = (float)width / height;
	const float Px = (2.0f * pixelNDCX - 1.0f) * aspectRatio * fov;
	const float Py = (1.0f - 2.0f * pixelNDCY) * fov;

	vec3 rayP{ Px, Py, -1.0f };

	vec3 origin{ 0.0f, 0.0f, 0.0f };

	float CameraToWorld[4][4]{
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f }
	};
	translate(CameraToWorld, vec3{ 0.0f, 0.8f, 4.5f });

	vec3 rayWorldOrigin{ dot(CameraToWorld[0], origin), dot(CameraToWorld[1], origin), dot(CameraToWorld[2], origin) };
	vec3 rayPWorld{ dot(CameraToWorld[0], rayP), dot(CameraToWorld[1], rayP), dot(CameraToWorld[2], rayP) };

	vec3 rayWorldDir{
		rayPWorld.x - rayWorldOrigin.x,
		rayPWorld.y - rayWorldOrigin.y,
		rayPWorld.z - rayWorldOrigin.z
	};

	return makeRay(rayWorldOrigin, rayWorldDir);
}

void traceImage(RTCScene scene, const std::vector<Material>& Materials, PPMImage& Color, uint32_t iteration)
{
	const uint32_t width = Color.getWidth();
	const uint32_t height = Color.getHeight();

	static const uint32_t bounces = 4;
	RandomSample sampler(iteration);

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			RTCRay cameraRay = makeCameraRay(x, y, width, height);
			Radiance currentColor(0.0f, 0.0f, 0.0f);
			Color.GetPixel(x, y, currentColor.x, currentColor.y, currentColor.z);
			Radiance Lo = currentColor + (pathTraceRayRecursive(scene, Materials, cameraRay, sampler, bounces));
			Color.SetPixel(x, y, Lo.x, Lo.y, Lo.z);
		}
	}
}
