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
static const float EPSILON = 0.00003f;
static const float gamma = 2.2f;

class RandomSample
{
	std::default_random_engine generator;
	std::uniform_real_distribution<float> distribution;

public:
	RandomSample() : distribution(0.0f, 1.0f) {}
	float next() { return distribution(generator); }
};

vec3 Trace(RTCScene scene, const std::vector<Material>& Materials, RTCRay& ray)
{
	std::default_random_engine gen;
	std::uniform_real_distribution<float> distr;

	vec3 color{ 0.0f, 0.0f, 0.0f };
	vec3 mask{ 1.0f, 1.0f, 1.0f };

	for (int bounce = 0; bounce < 8; ++bounce)
	{
		rtcIntersect(scene, ray);
		if (ray.geomID == RTC_INVALID_GEOMETRY_ID)
		{
			return color += mask * vec3{ 0.5f, 0.5f, 0.5f };
		}

		vec4 normal{ 0.0f, 0.0f, 0.0f, 0.0f };
		rtcInterpolate2(scene, ray.geomID, ray.primID, ray.u, ray.v, RTC_USER_VERTEX_BUFFER1, &normal.x, nullptr, nullptr, nullptr, nullptr, nullptr, 3);

		float rand1 = 2.0f * PI * distr(gen);
		float rand2 = distr(gen);
		float rand2s = std::sqrt(rand2);

		// create a local orthogonal coordinate frame centered at the hitpoint
		vec3 w = vec3{ normal.x, normal.y, normal.z };
		vec3 axis = fabs(w.x) > 0.1f ? vec3(0.0f, 1.0f, 0.0f) : vec3(1.0f, 0.0f, 0.0f);
		vec3 u = normalize(cross(axis, w));
		vec3 v = cross(w, u);

		/* use the coordinte frame and random numbers to compute the next ray direction */
		vec3 newdir = normalize(u * cos(rand1) * rand2s + v * sin(rand1) * rand2s + w * sqrt(1.0f - rand2));

		/* add the colour and light contributions to the accumulated colour */
		color += mask;

		/* the mask colour picks up surface colours at each bounce */
		mask *= vec3(Materials[ray.geomID].DiffuseColor[0], Materials[ray.geomID].DiffuseColor[1], Materials[ray.geomID].DiffuseColor[2]);

		/* perform cosine-weighted importance sampling for diffuse surfaces*/
		mask *= dot(newdir, vec3(normal.x, normal.y, normal.z));

		// update the ray origin
		ray.org[0] = ray.org[0] + ray.tfar * ray.dir[0] + EPSILON;
		ray.org[1] = ray.org[1] + ray.tfar * ray.dir[1] + EPSILON;
		ray.org[2] = ray.org[2] + ray.tfar * ray.dir[2] + EPSILON;

		// update the ray direction
		ray.dir[0] = newdir.x;
		ray.dir[1] = newdir.y;
		ray.dir[2] = newdir.z;

		ray.tnear = 0.0f;
		ray.tfar = std::numeric_limits<float>::max();
		ray.time = 0.0f;
		ray.mask = 0;
		ray.geomID = RTC_INVALID_GEOMETRY_ID;
		ray.primID = RTC_INVALID_GEOMETRY_ID;
	}

	return color;
}

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
	return pow(color, gamma) / PI;
}

float visibility(RTCScene scene, const vec3& o, const vec3& d)
{
	RTCRay shadowRay = makeRay(o, d);
	shadowRay.tnear = 0.001f;
	shadowRay.tfar = 1.0f;
	rtcOccluded(scene, shadowRay);
	return shadowRay.geomID ? 1.0f : 0.0f;
}

static Radiance traceRay(RTCScene scene, const std::vector<Material>& Materials, RTCRay& ray)
{
	Radiance outgoing = WorldGetBackground(ray);
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
		vec3 Li = Power / (distance * distance);
		outgoing = Li * shade(Materials, ray) * std::max(0.0f, dot(N, Wi)) * visibility(scene, P, toLight);
	}
	return outgoing;
}

static vec3 uniformSampleHemisphere(RandomSample& sampler)
{
	const float r1 = sampler.next();
	const float r2 = sampler.next();
	const float sinTheta = std::sqrtf(1.0f - r1 * r1);
	const float phi = 2.0f * PI * r2;
	const float x = sinTheta * std::cosf(phi);
	const float z = sinTheta * std::sinf(phi);
	return vec3(x, r1, z);
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

static Radiance pathTraceRayRecursive(RTCScene scene, const std::vector<Material>& Materials, RTCRay& ray, RandomSample& sampler, uint32_t pathDepth)
{
	if (pathDepth == 0)
	{
		return Radiance(0.0f, 0.0f, 0.0f);
	}

	Radiance outgoing = WorldGetBackground(ray);
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
		vec3 LiDirect = Power / (distance * distance) * visibility(scene, P, toLight) * std::max(0.0f, dot(N, Wi));

		outgoing += LiDirect;

		vec3 IndirectDiffuse(0.0f, 0.0f, 0.0f);
		uint32_t NumSamples = 16;
		float pdf = 1.0f / (2.0f * PI);
		for (uint32_t i = 0; i < NumSamples; ++i)
		{
			vec3 worldDirection = getBRDFRay(P, N, sampler);
			IndirectDiffuse += pathTraceRayRecursive(scene, Materials, makeRay(P, worldDirection), sampler, pathDepth - 1) / pdf * std::max(0.0f, dot(N, worldDirection));
		}

		outgoing += (IndirectDiffuse / (float)NumSamples) * shade(Materials, ray);
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

void traceImage(RTCScene scene, const std::vector<Material>& Materials, PPMImage& Color)
{
	const uint32_t width = Color.getWidth();
	const uint32_t height = Color.getHeight();

	static const uint32_t pathDepth = 2;
	RandomSample sampler;

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			RTCRay cameraRay = makeCameraRay(x, y, width, height);
			Radiance Lo = pathTraceRayRecursive(scene, Materials, cameraRay, sampler, pathDepth);
			Color.SetPixel(x, y, Lo.x, Lo.y, Lo.z);
		}
	}
}
