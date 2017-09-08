#pragma once
#include <string>
#include <vector>

#include <embree2/rtcore.h>

_declspec(align(16)) struct Material
{
	float DiffuseColor[3];
};

class TriangleMesh
{
	// Embree Data
	unsigned geomID = RTC_INVALID_GEOMETRY_ID;
	RTCScene scene = nullptr;

	// Vertex Streams
	struct Normal* n = nullptr;
	struct TextureCoord* uv = nullptr;

public:
	TriangleMesh() = default;
	~TriangleMesh();
	TriangleMesh(
		RTCScene InScene,
		const std::vector<float>& inP,
		const std::vector<float>& inN,
		const std::vector<float>& inUV,
		const std::vector<int>& inIndices, 
		size_t numTriangles, 
		size_t numVertices);
};

void LoadObjMesh(const std::string & Filename, RTCScene scene, std::vector<TriangleMesh*>& OutMeshes, std::vector<Material>& OutMaterials);
