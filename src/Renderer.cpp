#include "Renderer.h"

vec3 WorldGetBackground(const RTCRay& ray)
{
	return vec3{ 0.5f, 0.5f, 0.5f };
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
