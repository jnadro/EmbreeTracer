#pragma once

struct vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
};

struct vec4 { float x, y, z, a; };

