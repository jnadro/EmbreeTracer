#include <cmath>
#include <random>

#include "Renderer.h"

vec3 WorldGetBackground(const RTCRay& ray)
{
	return vec3{ 0.5f, 0.5f, 0.5f };
}

static const float PI = 3.14159265359f;
static const float EPSILON = 0.00003f;

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

		/* the mask colour picks up surface colours at each bounce */
		mask *= vec3(Materials[ray.geomID].DiffuseColor[0], Materials[ray.geomID].DiffuseColor[1], Materials[ray.geomID].DiffuseColor[2] );

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
	}

	return color;
}

vec3 CalculateColor(RTCScene scene, const std::vector<Material>& Materials, RTCRay& ray)
{
	vec3 color{ 0.0f, 0.0f, 0.0f };

	rtcIntersect(scene, ray);
	if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		color.x = Materials[ray.geomID].DiffuseColor[0];
		color.y = Materials[ray.geomID].DiffuseColor[1];
		color.z = Materials[ray.geomID].DiffuseColor[2];

		/*
		vec4 uv{ 0.0f, 0.0f, 0.0f, 0.0f };
		rtcInterpolate2(scene, ray.geomID, ray.primID, ray.u, ray.v, RTC_USER_VERTEX_BUFFER0, &uv.x, nullptr, nullptr, nullptr, nullptr, nullptr, 2);
		uvAOV.SetPixel(x, y, uv.x, uv.y, 0.0f);

		vec4 n{ 0.0f, 0.0f, 0.0f, 0.0f };
		rtcInterpolate2(scene, ray.geomID, ray.primID, ray.u, ray.v, RTC_USER_VERTEX_BUFFER1, &n.x, nullptr, nullptr, nullptr, nullptr, nullptr, 3);
		normalAOV.SetPixel(x, y, n.x*0.5f + 0.5f, n.y*0.5f + 0.5f, n.z*0.5f + 0.5f);
		*/
	}
	else
	{
		color = WorldGetBackground(ray);
	}

	return color;
}
