#pragma once

struct vec3
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

	inline vec3& vec3::operator*=(const vec3 &rhs) 
	{
		x *= rhs.x;
		y *= rhs.y;
		z *= rhs.z;
		return *this;
	}

	inline vec3& vec3::operator+=(const vec3 &rhs) 
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	inline vec3& vec3::operator-=(const vec3 &rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	inline vec3& vec3::operator*=(const float t) 
	{
		x *= t;
		y *= t;
		z *= t;
		return *this;
	}

	inline vec3& vec3::operator/=(const float t) {
		x /= t;
		y /= t;
		z /= t;
		return *this;
	}

	inline float length() const 
	{
		return std::sqrt(squaredLength());
	}
	inline float squaredLength() const 
	{
		return x * x + y * y + z * z;
	}
};

inline vec3 operator+(vec3 lhs, const vec3& rhs) 
{
	lhs += rhs;
	return lhs;
}

inline vec3 operator-(vec3 lhs, const vec3& rhs)
{
	lhs -= rhs;
	return lhs;
}


inline vec3 operator*(vec3 lhs, const vec3& rhs) 
{
	lhs *= rhs;
	return lhs;
}

inline vec3 operator*(vec3 lhs, const float& rhs) 
{
	lhs *= rhs;
	return lhs;
}

inline vec3 operator/(vec3 lhs, const float rhs) 
{
	lhs /= rhs;
	return lhs;
}


inline vec3 normalize(vec3 v) 
{
	return v / v.length();
}

inline vec3 cross(const vec3& v1, const vec3& v2) 
{
	return vec3((v1.y * v2.z - v1.z * v2.y),
		(-(v1.x * v2.z - v1.z * v2.x)),
		(v1.x * v2.y - v1.y * v2.x));
}

inline float dot(const vec3& v1, const vec3& v2) 
{
	return v1.x * v2.x +
		v1.y * v2.y +
		v1.z * v2.z;
}

inline float dot(const float row[4], vec3 v)
{
	return (row[0] * v.x + row[1] * v.y + row[2] * v.z + row[3] * 1.0f);
}

inline void translate(float matrix[4][4], const vec3& translation)
{
	matrix[0][3] = translation.x;
	matrix[1][3] = translation.y;
	matrix[2][3] = translation.z;
}

struct vec4 { float x, y, z, a; };

