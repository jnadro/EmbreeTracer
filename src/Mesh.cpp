#include <iostream>

#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

struct Vertex { float x, y, z, w; };
struct Normal { float x, y, z; };
struct TextureCoord { float u, v; };
struct Triangle { int v0, v1, v2; };
const size_t alignment = 16;

TriangleMesh* LoadObjMesh(const std::string & Filename, RTCScene scene)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	const bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, Filename.c_str());

	if (ret == false || shapes.size() < 1)
	{
		std::cerr << err << std::endl;
		return false;
	}

	// only deal with first shape.
	const tinyobj::shape_t& shape = shapes[0];

	const size_t numTriangles = shape.mesh.num_face_vertices.size();
	const size_t numVertices = attrib.vertices.size() / 3;

	// ineffcient
	std::vector<int> indices;
	indices.reserve(shape.mesh.indices.size());
	for (size_t i = 0; i < shape.mesh.indices.size(); ++i)
	{
		indices.push_back(shape.mesh.indices[i].vertex_index);
	}

	return new TriangleMesh(scene, attrib.vertices, attrib.normals, attrib.texcoords, indices, numTriangles, numVertices);
}

TriangleMesh::TriangleMesh(
	RTCScene InScene, 
	const std::vector<float>& p, 
	const std::vector<float>& normals, 
	const std::vector<float>& uvs,
	const std::vector<int>& indices,
	size_t numTriangles, 
	size_t numVertices)
	: scene(InScene)
{
	geomID = rtcNewTriangleMesh2(scene, RTC_GEOMETRY_STATIC, numTriangles, numVertices);

	{
		Vertex* vertices = (Vertex*)rtcMapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
		assert(vertices);
		for (size_t v = 0; v < numVertices; ++v)
		{
			vertices[v].x = p[3 * v + 0];
			vertices[v].y = p[3 * v + 1];
			vertices[v].z = p[3 * v + 2];
			vertices[v].w = 1.0f;
		}
		rtcUnmapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
	}

	if (uvs.size())
	{
		const size_t numBytes = uvs.size() * sizeof(float);
		assert((uvs.size() / 2) == numVertices);
		uv = static_cast<TextureCoord*>(_aligned_malloc(numBytes, alignment));
		for (size_t v = 0; v < numVertices; ++v)
		{
			uv[v].u = uvs[2 * v + 0];
			uv[v].v = uvs[2 * v + 1];
		}
		rtcSetBuffer2(scene, geomID, RTC_USER_VERTEX_BUFFER0, uv, 0, sizeof(TextureCoord), numVertices);
	}

	if (normals.size())
	{
		const size_t numBytes = normals.size() * sizeof(float);
		assert((normals.size() / 3) == numVertices);
		n = static_cast<Normal*>(_aligned_malloc(numBytes, alignment));
		for (size_t v = 0; v < numVertices; ++v)
		{
			n[v].x = normals[3 * v + 0];
			n[v].y = normals[3 * v + 1];
			n[v].z = normals[3 * v + 2];
		}
		rtcSetBuffer2(scene, geomID, RTC_USER_VERTEX_BUFFER1, n, 0, sizeof(Normal), numVertices);
	}

	{
		Triangle* triangles = (Triangle*)rtcMapBuffer(scene, geomID, RTC_INDEX_BUFFER);
		assert(triangles);

		size_t index_offset = 0;
		for (size_t i = 0; i < numTriangles; ++i)
		{
			triangles[i].v0 = indices[3 * i + 0];
			triangles[i].v1 = indices[3 * i + 1];
			triangles[i].v2 = indices[3 * i + 2];
		}

		rtcUnmapBuffer(scene, geomID, RTC_INDEX_BUFFER);
	}
}

TriangleMesh::~TriangleMesh()
{
	if (n)
	{
		_aligned_free(n);
	}
	if (uv)
	{
		_aligned_free(n);
	}
}
